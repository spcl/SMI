/*
    Gather benchmark: each rank sends its contribution (sequence of numbers) for the gather.
    The root checks the correctness of the result
*/

#include <smi.h>


__kernel void app(const int N, char root, __global char *mem, SMI_Comm comm)
{
    SMI_GatherChannel  __attribute__((register)) chan= SMI_Open_gather_channel(N,N, SMI_INT,0, root,comm);
    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    int to_send=(my_rank==root)?0:my_rank*N;    //starting number
    char check=1;
    for(int i=0;i<loop_bound;i++)
    {
        int to_rcv;
        SMI_Gather(&chan,&to_send, &to_rcv);
        to_send++;
        if(my_rank==root)
            check&=(to_rcv==i);
    }
    *mem=check;
}
