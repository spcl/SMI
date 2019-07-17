/**
    The purpose of this microbenchmark is to highlight
    the fact that in SMI more than one collective can be executed
    simultaneously.

    To show this, we implemented a microbenchmark in which we have to execute two broadcasts.

    In the first application (sequential_collectives) they are executed one afther the
    other.

    In the second application (simultaneous_collectives) they are executed together.
*/


#define BUFFER_SIZE 256 //provisional
#include <smi.h>
#include "smi-generated-device.cl"

__kernel void sequential_collectives(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    float start_float=1.1f;
    int start_int=1;
    char check=1;
    //first execute the reduce
    SMI_BChannel  __attribute__((register)) bchan_float= SMI_Open_bcast_channel(N, SMI_FLOAT,0, root,comm);
    for(int i=0;i<N;i++)
    {
        float to_comm;
        if(my_rank==root)  to_comm=start_float+i;
        SMI_Bcast(&bchan_float,&to_comm);
        if(my_rank!=root)
            check&=(to_comm==start_float+i);
    }

    //then the broadcast
    SMI_BChannel  __attribute__((register)) bchan_int= SMI_Open_bcast_channel(N, SMI_INT,1, root,comm);
    for(int i=0;i<N;i++)
    {
        int to_comm;
        if(my_rank==root) to_comm=start_int+i;
        SMI_Bcast(&bchan_int,&to_comm);
        if(my_rank!=root)
            check &= (to_comm==start_int+i);
    }
    *mem=check;
}


__kernel void simultaneous_collectives(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    float start_float=1.1f;
    int start_int=1;
    char check=1;
    //first execute the reduce
    SMI_BChannel  __attribute__((register)) bchan_float= SMI_Open_bcast_channel(N, SMI_FLOAT,2, root,comm);
    SMI_BChannel  __attribute__((register)) bchan_int= SMI_Open_bcast_channel(N, SMI_INT,3, root,comm);
    for(int i=0;i<N;i++)
    {
        float to_comm;
        if(my_rank==root)  to_comm=start_float+i;
        SMI_Bcast(&bchan_float,&to_comm);
        if(my_rank!=root)
            check&=(to_comm==start_float+i);

        int to_comm_int;
        if(my_rank==root) to_comm_int=start_int+i;
        SMI_Bcast(&bchan_int,&to_comm_int);
        if(my_rank!=root)
            check &= (to_comm_int==start_int+i);

    }
    *mem=check;
}
