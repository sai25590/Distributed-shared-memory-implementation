       #include "dsm.h"	
       #include <sys/mman.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #include <stdio.h>
       #include <stdlib.h>
       #include <unistd.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include<signal.h>
	#include<ctype.h>
	#include<string.h>
	#include<errno.h>
	#include<semaphore.h>
	

typedef struct sockaddr SA;
typedef void Sigfunc(int);
static void handler(int sig, siginfo_t *si, void *unused);

/*If a connection request arrives with the queue full,
the client may receive an error with an indication
of ECONNREFUSED*/
#define LISTENQ 5

/*our server is going to listen in the following port.*/
#define SERV_PORT 1050 

/*Maximum number of character that can be read from
the connection.*/
#define MAXLINE 4096

  
       #define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
       
	#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)




        void *addr;		
	int sockfd;
	int sem = 1;

	pthread_mutex_t *mutex1;
	pthread_t listen_thread;

int cli_code(char  *ip, int port)
{
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	inet_pton(AF_INET,ip,&servaddr.sin_addr);
	//printf("Connnecting to port number %d\n", port);
	while( connect(sockfd,(SA*)&servaddr,sizeof(servaddr) ) != 0);
	//printf("Connection Established...\n");
	return sockfd;
}

int serv_code(int port)
{
	int listenfd,connfd;
	pid_t childpid;
	socklen_t clilen;
	int val = 1;
	struct sockaddr_in cliaddr, servaddr;
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); 
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(port);
	bind(listenfd,(SA*)&servaddr,sizeof(servaddr));
	listen(listenfd,LISTENQ);
	for( ; ; )
	{
		clilen=sizeof(cliaddr);
		//printf("Waiting in port %d\n",port);
		connfd=accept(listenfd,(SA*)&cliaddr,&clilen);
		close(listenfd);		
		return connfd;
	}
}


void func_listen( void *ptr)
	{		
		int connfd = (int ) ptr;	
		char buff[4096];
		int n, pageno;
		int temp;
       
		for( ; ; )
		{			
			n=read(connfd,&pageno,sizeof(int));
			pthread_mutex_lock( &mutex1[pageno] );
			mprotect(addr + 4096 * pageno, 4096,PROT_READ);	
			memcpy(buff, addr + 4096 * pageno, 4096);
			mprotect(addr + 4096 * pageno, 4096,PROT_NONE);	
			write(connfd,buff,sizeof(buff));
			pthread_mutex_unlock( &mutex1[pageno] );

		}
		close(connfd);
	}
         
      void * getsharedregion()
	{
		return addr;		      
	}

	void initializeDSM(int ismaster, char * masterip, int mport, char *otherip, int oport, int numpagestoalloc)
	{
	//initialzing a thread which will listen in a socket	

	int port;
	char *ip;
	int s, i;


	//Memory mapping - start
         addr = mmap(0x40000000, 4096 * numpagestoalloc, PROT_NONE, MAP_ANON | MAP_SHARED, -1, 0);
	//printf(" Base address: 0x%lx\n",(long) addr);
	if (addr == MAP_FAILED)
 	                  handle_error("mmap");

	if(ismaster)
	{
		mprotect(addr, 4096 * numpagestoalloc/2,PROT_READ | PROT_WRITE);	
	}
	else
	{
		mprotect(addr + 4096 * numpagestoalloc/2 , 4096 * numpagestoalloc/2,PROT_READ | PROT_WRITE);	
	}


	//printf("Memory allocated....\n");	

	//Memory mapping - end

	// creating mutexes pthread_mutex_t *mutex1 = PTHREAD_MUTEX_INITIALIZER

	/*pthread_mutex_t m1;

	
	*/
	mutex1 = (pthread_mutex_t *) malloc(numpagestoalloc * sizeof(pthread_mutex_t)); 
		for(i = 0; i < numpagestoalloc; i++)
			pthread_mutex_init(&mutex1[i], NULL);


	int connfd;
	if(ismaster) {
		connfd = serv_code(mport);
		sockfd = cli_code(masterip,oport);
	}
	else {
		sockfd = cli_code(masterip,mport);
		connfd = serv_code(oport);
	}
	

//thread - start
    	s = pthread_create (&listen_thread, NULL, (void *) &func_listen, (void *) connfd);	
	 if (s != 0)
             handle_error_en(s, "pthread_create");
//thread - end


//Handling SIGSEGV - start
	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
    	sigemptyset(&sa.sa_mask);
    	sa.sa_sigaction = handler;
    	if (sigaction(SIGSEGV, &sa, NULL) == -1)
        	handle_error("sigaction");

//Handling SIGSEGV - end


       }
static void handler(int sig, siginfo_t *si, void *unused)
{


  int pageno = (int) (si->si_addr - addr)/4096;
	pthread_mutex_lock( &mutex1[pageno] );          		
//  printf("page num = %d\n", pageno);

  
//  printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);


	int port, n;
	char *ip;
	char recvline[4096];
	int temp;
	
	
	write(sockfd,&pageno,sizeof(int));	
	
	
	if((n = read(sockfd,recvline,MAXLINE)) == 0)
	{
		printf("Read error\n");
		exit(0);
	}
	else
	{
			mprotect(addr + 4096 * pageno, 4096,PROT_WRITE);				
			memcpy(addr + 4096 * pageno, recvline , 4096);
			mprotect(addr + 4096 * pageno, 4096,PROT_READ | PROT_WRITE);				
	}
         
	pthread_mutex_unlock( &mutex1[pageno] );
}

void destrongDSM()
{
	close(sockfd);	
	pthread_join(listen_thread, NULL);			
}
