/**
	Pipeline double rail: we have two pipelines running across multiple devices
	Devices are interconnected using two network channels per pair. Under balanced routing,
	the two streams should use two different network ports

*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#include <smi.h>

__kernel void app(__global char* mem, const int N,SMI_Comm comm)
{
    char check1=1, check2=1;

    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);

    //each rank increments by one
    int expected1 = num_ranks-1;
    int expected2 = num_ranks;
    SMI_Channel chan_send1=SMI_Open_send_channel(N, SMI_INT, my_rank+1, 0, comm);
    SMI_Channel chan_send2=SMI_Open_send_channel(N, SMI_INT, my_rank+1, 1, comm);
    SMI_Channel chan_rcv1=SMI_Open_receive_channel(N, SMI_INT, my_rank-1, 0, comm);
    SMI_Channel chan_rcv2=SMI_Open_receive_channel(N, SMI_INT, my_rank-1, 1, comm);

    for(int i=0;i<N;i++)
    {
	    int data1, data2;
	    if(my_rank > 0){
		    SMI_Pop(&chan_rcv1, &data1);
		    SMI_Pop(&chan_rcv2, &data2);
		    // printf("Rank %d received data %d\n",my_rank,i);
		    data1++;
		    data2++;
	    }
	    else{
		    data1=i;
		    data2=i+1;
        }
        if(my_rank < num_ranks -1){
		    SMI_Push(&chan_send1, &data1);
		    SMI_Push(&chan_send2, &data2);
		    // printf("Rank %d sent data %d\n",my_rank,i);
        }
	    else{
		    check1 &= (data1==expected1);
		    check2 &= (data2==expected2);
		    // printf("Last rank received %d (exp: %d), and %d (exp %d)\n",data1,expected1,data2,expected2);
		    expected1++;
		    expected2++;
	    }
    }
    *mem=check1&check2;
}
