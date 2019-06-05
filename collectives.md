The idea is to present the interface for collective communications.
In discussing the interface, some implementation issues/descision pop-up, so maybe it is worth to mention them.  
Collectives require the introduction of ad-hoc channels.

**General question**: we have to decide what to do with communicators: if we want to have them, or if we want to 
resort to tags (that in SMI indentify communication endpoints and are used to layout hardware connections). In the following I've used tags

- **Broadcast**, is the simplest one.
    - Open channel: `SMI_BChannel SMI_Open_broadcast_channel(uint count, SMI_Datatype type, char root, char my_rank, char tag);`
    - broacast: `void SMI_Broadcast(SMI_BChannel *chan, void* data)`

    We need to know the rank (in the open channel or in the communication primitive) so that we can distinguish if we want to push or pop data


	Rendezvous: each non-root rank should send a "ready to receive" message to the root, to avoid intermixing of different collectives. This could occur when the root of the broadcast changes

- **Reduce**: data elements streamed by each rank will be combined and produced to the root
    -  Open channel: `SMI_RChannel SMI_Open_receive_channel(uint count, SMI_Datatype type, SMI_Op operator, char root, char my_rank, char tag);`
    -  Reduce: `SMI_Reduce(SMI_RChannel *chan, void* data)`

    Here there are two implementation issues to address:
    
    - who will perform the actual reduce? It could be done  on the application side (inside the `SMI_Reduce` or on the network side (inside the CK_R or other support entity)
    - how to handle the case in which `count>1`: the reducer could receive data from the various rank in any order (for example first all the data from rank 0 and then all the data from rank 1'' remember that the QSFP has when the data arrives). Therefore it need to buffer it in order to produce the correct result


- **Scatter**: should be ok, the interface resembles the one of MPI, again with the open channel and communication primitive

- **Gather**: 
    - Open_Channel: ` SMI_Open_gather_channel(count, data_type, root, tag, myrank);` 
    - Gather: `SMI_Gather(SMI_GChannel *chan, void* data)`

    Here we have a problem similar to the `reduce` case: data may arrive to the root in any order but it must be streamed in the correct one. So first the `count` elements from the first rank, than the  elements from the second and so on... This can not work in our current implementation, which essentially use an `eager` protocol for transferring the data. So in this case, one can consider to adopt `rendezvous` protocl
