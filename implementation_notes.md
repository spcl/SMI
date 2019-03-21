# Implementation notes
These are technical notes regarding the implementation. They cover both the API and internal details.
This notes must be intended as *draft*, but can be used as starting point for writing parts of the paper.

## API
The High-Level interface offers to the user the abstraction of *communication channel* to represent a 
point-to-point (P2P) communication . A channel connects two endpoints of the computation and it is used to transfer **a single** message.
Endpoints are uniquelly identified by the *rank* of the sender/receiver and a *tag*. Therefore the communication channel is idenfied by:

`<src rank, dst rank, tag, data_type, message size >`

the tag is used to distinguish different messages exchanged between two ranks.
There is no matching on the receiver side: only the communication tag is used to deliver the message. Therefore, if a computation must receive two different messages (possibly from different senders), it has to use two channels with different tags.

*Please note:* the fact that we are offering channels doesn't imply that we are providing connection base communications.

**Assumptions**

- tags are known at compile time, which means that **they do not depend from the rank**. This is done to generate efficient hardware and to avoid the generation of a different code (and therefore a different synthesized design) for each rank in the case of SPMD programs;
- to each FPGA will be assigned a single rank number.


The channel is represented by a *channel descriptor*. The user can declare a channel by specifing the current rank of the computation, the pair-rank, tag, message size, message data type and type of the channel (send/receive) (**this interface is provisional**)

```C++
chdesc_t open_channel(char my_rank, char pair_rank, char tag, uint message_size, data_t type, operation_t op_type)
```

*Please note*:

- `my_rank` and `pair_rank` are needed for routing reasons;
- we can also have a `close_channel`, but for the moment being is useless.

*Channels* are accessible through a cycle-by-cycle interface: computations can `push` or `pop` data to/from communication channel, one data element per clock cycle.
```C++
void push(chdesc_t *chan, void* data)
```

```C++
void pop(chdesc_t *chan, void *data)
```
There is no need to specify additional parameters, since the useful information are stored in the channel descriptor.

####Example
Suppose that we want to send a message composed by N integers. Sender and receiver will use a communication channel having tag 0.
The code of the sender will be:
```C++
    ...
    chdesc_t chan=open_channel(my_rank,dest_rank,0,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=array[i];
        push(&chan,&data);
    }
    ...

```

The code of the receiver:
```C++
    ...
    chdesc_t chan=open_channel(my_rank,src_rank,0,N,INT,RECEIVE);
    for(int i=0; i< N;i++)
    {
        int rcvd;
        pop(&chan,&rcvd);
        array[i]=rcvd;
    }
    ...

```



###Communication mode
Here we have to distinguish between the *globa*l message send/receive (i.e. the transferring of the whole message content) and the single push/pop (i.e. the transferring of a single data element).

FIFO buffers, that are used to actually implement our communication channels, provide a certain buffer space (tunable at compile time). 
Data `push`es exploits this buffer space: therefore the single push (and hence message send) may complete if there is sufficient buffer space in the FIFO buffers and can complete before the corresponding `pop`s are performed. If the buffer space is not sufficient, the `push` will stall  until the receiver starts to `pop` from the channel.

The send is *non-local*: it can be started wheter or not the receiver is ready to receive,but its completion may depend on the occurence of the matching `pop`s.

*Note*: this seems to be similar to MPI *standard* mode.

### Correctness
We assume that the correctenss of the program is guaranteed by the user, i.e. the sequence of sends/receives does not deadlock.

Given that we assume that the receiver is ready to receive (or that there is sufficent buffer space), we use an *eager* protocol to perform the communication: data is sent as soon as possible. There is no rendez-vous between sender and receiver.
This is done:

- for the sake of removing additional overhead (in terms of performance and hardware) imposed by randez-vous;
- because it is closer to the channel philosophy and HLS way of writing programs: if the receiver is ready to receive, a message element is sent at each clock-cycle and the send is actually pipelined.


### Naming
I would like to give a name to this library, in order to use it as prefix to function calls and constatns. What about SMI: Streaming Message Interface?


##Implementation
These notes are draft ideas on the implementation


### Communicators
In our architecture we have two types of communicator kernels:

-  `CK_S`, sender communicator kernels: they receive data from applications (and not only) and forward it into a network channel;
-  `CK_R`, receiver communicator kernels: they receive data from network and forward it to applications (or to `CK_S` as we will see).

**Design choice**: should we have a single big CK_S and CK_R that handle all the I/O channels or having 4 of them? The former is simpler to implement, but will result in a deeper logic and an higher probability of stalls. Suppose that we have one of the I/O channels that is full: the CK_S will be not able to handle any other send because of this. This penalizes performances.

Communication channels between applications and CK are known at compile time. An internal switching table is used to properly identify (inside an FPGA) endpoints and how they
are connected to CKs. Therefore, there will be:

- for sending channels, an *internal* switching table that maps `TAG -> ck_s_channel_id`. Here by channel we are referring to FIFO buffers. This `channel_id` is used by the `push` method to forward 
   ready to go packets
- for receiving channels, an *internal* switching table that maps `TAG -> ck_r_channel_id`. The `channel_id` is used by `CK_R` to forward data to the right application and from `pop` to receiving this data.

This internal switching tables must be known at compile time. This is the reason why TAGs are compile time constant. Supposing that TAGs start from zero and are incremental, **possibly** the compiler will be able to create efficient hardware (i.e. connecting the different pushes/pops to the right channel).


**Problem**: we have to define how communication channels/tag are bound to CKs. Let's consider the case of CK_S. Suppose that we have four of them, because all QSFP are connected. Decide to wich CK_S and endpoint is connected depends on the phisical topology, and cannot be done (efficiently) buy simply round robin between them (well....consider the first problem in the routing subsection)

**NOTE**: we can have only one QSFP between two endpoints. In the case that two FPGAs are connected using multiple QSFPs, the user can exploit them by declaring and using multiple channels with differnt TAGs


Communicators will be also connected between each other for routing issue (see the next section). In particular:
- all CK_S are interconnected between each other (12 channels)
- CK_R are connected to CK_S. Here we can decide if all of the are connected to all CK_S or just to one

## Routing

As reference architecture we will use the one in which there is one `CK_S` and  one `CK_R` for each QSFP module.

To recall, communications between application occur using channels identified by `<src rank, dest rank, tag>` (data type and message size are not important in this context).

We can have two possible way of implementing the actual data transfers, that will result in different FPGA designs and routing problems. Feedbacks are appreciated:

1.  the schema considered so far is the one in which the different `CK_S` are interconnected. Therefore an application endpoint (i.e. a push or pop to/from channels) can be connected to anyone of these `CK_S`. The `CK_S` will receive packets, will look at their destination, compute the route and then will forward them to the QSFP at which it is attached or to another `CK_S` in the FPGA. The routing tables used to compute the route are stored into the FPGA RAM. Similar reasoning applies to the receive side (`CK_R`): a `CK_R` receives a packet that can be directed to one of the endpoints directly connected to it. In this case it will forward the packet to the application. However, the packet could be directed to an endpoint connected to another `CK_R` (so it will forward the packet to it) or to another machine (so it will forward the packet to another `CK_S`) 

2. another possibility is that `CK_S` and `CK_R` are not interconnected each other. In this case we have to compute the full routes between two endpoints and generate the proper hardware. For example, if we know that our application send data to `rank 1` and this rank is connected to `QSFP 0`, then the application endpoint will be connected to the `CK_S` attached to `QSFP 0`. Similar reasoning should apply also for the receiver side. In this case, communication kernels act only as interface: they take data from incoming channel and forward it. They don't need a routing table (or at least is simplified). All the routes must be pre-computed before compiling (the user will need to specify a list of them)

What are the difference between this two solutions?  (*Also here, if something come to mind please tell me*)

In my opinion, they are equivalent in terms of expressiveness. In the sense: any communication topology that you can express with solution 1 you can express also with 2.
**However** I believe that solution 1 is more flexible. If, for example, the physical topology changes (because someone switched cables or because we are using different nodes), with 1 it would be sufficient to recompute the routing tables, while with 2 it could be the case that you need to recompile everything.
Another point is that (if we want to provide this possibility), the rank of the receiver can be unknown at compile time with 1.

**So** I am more incline to consider solution 1 even if it is slightly more complicated. But I'm open to discussions.


**At the end we've choosen solution 1**:
- so the user doesn't have to specify all the channels
- more flexible and robust to changes in the topologies and so on...


So, considering solution 1., let's see what we need for the routing.

In input we have the physical topology, that is a list  of point-to point connections:
```
<nodename>:<device name>:<channel_id> - <nodename>:<device name>:<channel_id>

```
E.g:
```
fpga-0014:acl0:ch0 - fpga-0014:acl1:ch0
fpga-0014:acl0:ch3 - fpga-0014:acl1:ch3
```

Each channel is bi-directional. Cluster nodes goes from `fpga-0001` to `fpga-0016`. 
In each node we have 2 FPGAs (`acl0` and `acl1`). Each FPGAs has 4 QSFPs, so channels goes from `ch0` to `ch3`. We can think at them as a list of numbers. Possibly, not all the QSFPs of an FPGA are connected.


On each FPGA we will have a CK_S and CK_R for each used QSFP. Let's analyze what happens on the sender and receiver side.

### CK_Ss

Let `$N_Q$` be the number of used QSFPs. Each CK_S has a certain number of input/output connections:

 - **Input**: 
     -  connections from application endpoints;
     -  `$N_Q-1$` input channels from other CK_S;
     -  a connection with its own paired CK_R: this is used for multi-hop routes;
 - **Output**:
     - the connections with the QSFP;
     - `$N_Q-1$` output channels to other CK_S;
     - a connection with its own paired CK_R: this is used for local sends.

For the *internal routing*, that is the connections between application endpoints and CK_S is clearly managed at compile time. These internal connections, are managed with an array of channels (FIFO buffers) indexed using an internal routing table (which is accessed using the TAG).
Being the TAG known at compile time, this allows the compiler to create the right hardware connection.
We are using the array of channels and internal routing table, simply because in this way the `push`/`pop` implementation can be generic.
Otherwise each one use its own channel and we have to generate the code in such a way that on each `push`/`pop` occurence it is replace the write to the proper channel.  
For some details about code generation, check issue

**How to associate the channel endpoints to the CK_S**: for the moment being we can use any logic, even Round Robin.


For the *external routing*, each CK_S will have its own routing table. This will allow to reach any other *rank* which is currently involved in the computation. The routing table will be a simple array of that maps `$rank \rightarrow output_port$`, where output port is:

- 0, forward to network (QSFP)
- 1, send to the connected CK_R (local send)
- 2,...,`$N_Q$`, send to other CK_S

The routing table will be loaded at run-time, from DRAM.
Possibly, the host compute it on the fly (considering the used topology) and copies it to device.


### CK_Rs

Each CK_R has the following connections:

- **Input**: 
     -  connections from the QSFP;
     -  `$N_Q-1$` input channels from other CK_R;
     -  a connection with its own paired CK_S: this is used for local sends;
 - **Output**:
     - the connections with the application endpoints;
     - `$N_Q-1$` output channels to other CK_R;
     - a connection with its own paired CK_R: this is used for multi-hops routes.

As in the case of CK_S, the internal routing CK_R -> application endpoints, is done at compile time using an array of internal channel.

For the *external routing*, each CK_R has its own table. This time it is indexed by using pairs `$(rank,tag) \rightarrow output_port$`, where output port is:

- 0, send to the connected CK_S (remote send)
- 1, ...,`$N_Q-1$`, send to other CK_R (the application endpoint is not connected to this CK_R)
- other indexes: send to application

Again, this routing table will be loaded from DRAM.
*Therefore* to generate this routing table, we need to known the internal routing. This is known at compile time, we have to decide how to provide it to the function that computes the routes.


##Code Generation

** We will not handle this now. These are just notes/idea on how we should handle this**

What we need from the user is the number of TAGs used into his own program:
- this will be a single number for SPMD program
- multiple numbers for MPMD program

Optionally, she can input also the number of used QSFPs

Where we need code generation?

- internal mapping application endpoint -> CK_S and CK_R -> endpoint. We can think that this is generated in an header file (something linke `smi_defines.h`) that is then included in the user program. This header will include an array with this internal_routing, constants (e.g. the number of used rank), channels definition (I/O channels, channels between endpoints and CKs, channels between CKs)

- CK_Sender, CK_receivers: will need to be generated for the sake of interconnecting them properly with other pair and to application endpoints. Consider for example the pseudocode of a CK_S. It is organized as a `while(true)` loop that reads from its input channel

```C++
    const char num_sender=2+1; //number application endpoint + CK-pairs (in this case there is no CK_R attached)
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    while(1)
    {
        switch(sender_id)
        {
            case 0:
                mess=read_channel_nb_intel(channel_to_ck_s[0],&valid);
            break;
            case 1:
                mess=read_channel_nb_intel(channel_to_ck_s[1],&valid);
            break;
            case 2:
                mess=read_channel_nb_intel(channel_interconnect_ck_s[1],&valid);
            break;
            //....
        }

        if(valid)
        {
            char idx=external_routing_table[GET_HEADER_DST(mess.header)];
            switch(idx)
            {
                case 0:
                    write_channel_intel(io_out_0,mess);
                    break;
                case 1:
                    write_channel_intel(channel_interconnect_ck_s[0],mess);
                    break;
                //...

            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }

```

The first `switch` must be properly generated by considering the application endpoints that are connected to this particular CK_S


Clearly, for MPMD programs we will have multiple generated files in output


