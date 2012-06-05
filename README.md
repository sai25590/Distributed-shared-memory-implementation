Distributed-shared-memory-implementation
========================================

Description:
    
In this assignment I wrote my own distributed shared memory
implementation.  The basic idea is that you create (through software)
the illusion that two processes on different machines share the same
address space.  So when a process writes to a memory location in a
page that resides on a remote machine, my implementation asks
my other implementation on the remote machine for a copy, the remote
machine should protect its page, and then the local machine maps
the page's data into its own address space.  If the remote machine
were to access to the page, the same protocol transfers the page
back.
    
My implementation implements the process consistency memory
model (x86 memory consistency model --- (writes from one processor need
to be seen in the same order by the other machine). 

Thats good for description, but how to run?

For linux platforms:
 
To run a test case type
./test master masterip slaveip testcase_number   on the master machine and
./test slave masterip slaveip testcase_number   on the slave machine where
masterip == ip address of master machine
slaveip == ip address of slave machine
testcase_number is between 0 and 5.

