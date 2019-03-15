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

### Routing

We have given a phisical topology of point-to point connections:
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

Concerning the sender side

On each FPGA we will have a CK_S and CK_R for each QSFP:
- application endpoints are connected to CK_S (to send data) and CK_R (to receive data)
- connection are known at compile time
- (internally connection are stored in a routing table tag-> channel_to_ck_s)


Possiamo evitare gli hop interni? Andando a collegare opportunamente le applicazione ai CK_S?
Non credo, da pensarci. Questo potrebbe semplificare?
Che succede se la rete non e' simmetrica? 



Each CK_S is connect to one QSFP and to the other CK_S.
Furthermore it will receive data from a CK_R.
Each CK_S has its own routing table: (dest rank, tag) -> output_port (so the QSFP or another CK_S)

On the other side CK_R receive data that is directed to an application to which they are directly connected (E SE NON LO SONO CONNESSE?), or to another CK_S if we have to send it to another node.

How internal channels are assigned to `$CK_S$` or `$CK_R$` is known at compile time.
I think that it is not 100% guaranteed that we can assign them in a clever way. Suppose that 




To each communication channel we can associate one CK_S and one CK_R.

**Assumption**:
- each rank must be able to reach all the other ones
- 

**Problem**:  this must be done according to the application requirements (i.e. knowing that app on rank 0, fpga 0 will communicate with rank 1, fpga 1) or we want to guarantee an all-to-all connections?

The second solution is more flexible, even if it will possibly result in performance impairment.
In the sense, it is more convienient to associate the endpoint to the closest CK_S, but it is probably not general? In the sense that if I want to do this, I have to know who communicates with whom at compile time.




**Problem**: a different problem is that not all QSFP are connected so we may have less CKs than 4



