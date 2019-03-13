# Implementation notes
These are technical notes regarding the implementation. They cover both the API and internal details.
This notes must be intended as *draft*, but can be used as starting point for writing parts of the paper.

## API
The interface offer to the user the abstraction of *communication channel* to represent a 
point-to-point (P2P) communication . A channel connects two endpoints of the computation and it is used to transfer **a single** message.
Endpoints are uniquelly identified by the *rank* of the sender/receiver and a *tag*. Therefore the communication is idenfied by:

`<src rank, dst rank, tag, data_type, message size >`

the tag is used to distinguish different messages exchanged between two ranks.
There is no matching on the receiver side: only the communication tag are used to deliver the message. Therefore, if a computation must receive two different messages (possibly from different senders), it has to use two channels with different tags.

*Please note:* the fact that we are offering channels doesn't imply that we are providing connection base communications.

**Assumptions**

- (user writes SPMD programs) -- not sure about this;
- tags are known at compile time, which means that **they do not depend from the rank**. This is done to generate efficient hardware and to avoid the generation of a different code (and therefore a different synthesized design) for each rank in the case of SPMD programs;
- only a rank will exist on a given FPGA.


The channel is represented by a *channel descriptor*. The user can declare a channel by specifing the current rank of the computation, the pair-rank, tag, message size, message data type and type of the channel (send/receive) (**PROVISIONAL**)

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
Here we have to distinguish between the global message send/receive (i.e. the transferring of the whole message content) and the single push/pop.


FIFO buffers that are used to actually implement our communication channels provide a certain buffer space (tunable at compile time). 
Data push exploits this buffer space: therefore the single push (and hence message send) may complete if there is sufficient buffer space in the FIFO buffers and can complete before the corresponding pop is performed. Otherwise, the push will not complete until the `pop` is started.
The send is *non-local*: it can be started wheter or not the corresponding receive has been posted but its completion may depend on the occurence of the matching receive.

*Note*: this seems to be similar to MPI *standard* mode.

### Correctness
We assume that the correctenss of the program is guaranteed by the user.
May program rely on buffering? Consider the case of 2D-stencil for example....


Given that we assume that the receiver is ready to receive, we use an *eager* protocol to perform the communication: data is sent as soon as possible. There is no rendez-vous between sender and receiver.
Why this:

- for the sake of removing additional overhead (in terms of time and hardware) imposed by randez-vous;
- because it is closer to the channel philosophy and HLS way of writing programs: if the receiver is ready to receive, a message element is sent at each clock-cycle and the send is actually pipelined.


### Naming
What about SMI: streaming message interface?