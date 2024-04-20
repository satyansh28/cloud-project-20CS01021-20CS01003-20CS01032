# Contributors
1. Satyansh Shukla - 20CS01021
2. Prateek Kr. Singh - 20CS01003
3. Prathamesh Bhos - 20CS01032

# Theory
**Lamport’s Distributed Mutual Exclusion Algorithm** is a permission based algorithm proposed by Lamport as an illustration of his synchronization scheme for distributed systems. In this scheme, permission based timestamp is used to order critical section requests and to resolve any conflict between requests. In Lamport’s Algorithm critical section requests are executed in the increasing order of timestamps i.e a request with smaller timestamp will be given permission to execute critical section first than a request with larger timestamp.


# Implementation
Our implementation is a C++ realization of Lamport's Distributed Mutual Exclusion algorithm where N number of processes share a critical section which only one should access at a time.Our implementation uses sockets to communicate using the TCP protocol to exchange messages.These messages are of three kinds:
Request(Q): A process which wants to access the critical section sends this message to every other peer,requesting access to critical section.
Reply(R): A process responds to the request message with a reply after adding the request in it's priority_queue.
Open(O): A process sends the open message when it releases the critical section

For ordering we use timestamps produced by Lamport's logical clock,we have used atomic integers to synchronise the sender and receiver threads.We store requests in a min heap according to smallest timestamp first.

## Usage

To run the program with N processes,first you need to compile the c++ code using g++(or clang):

`g++ lamport_exclusion.cpp`

Following which you need to run it in N terminals using the following command(in Linux):

`./a.out`

Following this you will need to enter the port to listen on,the number of other peers in the network and their port numbers.After the above you will have a menu driven program to request critical section.

NOTE: We have added an 8 seconds delay when a process enters critical section before it releases the critical section.

[Sample image:](https://drive.google.com/file/d/1Ki96f-CroIVWf1Xbu8uP6L2uIZelfMAM/view?usp=sharing)