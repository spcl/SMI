/**
	Pipeline example

*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#include <smi.h>

__kernel void app(__global char* mem, const int N,SMI_Comm comm)
{
    char check=1;

    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    
    SMI_Channel chan_send=SMI_Open_send_channel(N, SMI_INT, my_rank+1, 0, comm);
    SMI_Channel chan_rcv=SMI_Open_receive_channel(N, SMI_INT, my_rank-1, 0, comm);

    for(int i=0;i<N;i++)
    {
	int data_to_send;
	if(my_rank > 0){
		SMI_Pop(&chan_rcv, &data_to_send);
	}
	else
		data_to_send=i;
        if(my_rank < num_ranks -1)
		SMI_Push(&chan_send, &data_to_send);
	else{
		check &= (data_to_send==i);
	}
    }
    *mem=check;
}
