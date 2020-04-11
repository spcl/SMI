/**
	Pipeline double rail with vector data type: we have two pipelines running across multiple devices
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
    float4 expected1 = num_ranks-1;
    float4 expected2 = num_ranks;
    SMI_Channel chan_send1=SMI_Open_send_channel(N, SMI_FLOAT4, my_rank+1, 0, comm);
    SMI_Channel chan_send2=SMI_Open_send_channel(N, SMI_FLOAT4, my_rank+1, 1, comm);
    SMI_Channel chan_rcv1=SMI_Open_receive_channel(N, SMI_FLOAT4, my_rank-1, 0, comm);
    SMI_Channel chan_rcv2=SMI_Open_receive_channel(N, SMI_FLOAT4, my_rank-1, 1, comm);

    for(int i=0;i<N;i++)
    {
	    float4 data1, data2;
	    if(my_rank > 0){
		    SMI_Pop(&chan_rcv1, &data1);
		    SMI_Pop(&chan_rcv2, &data2);
		    // printf("Rank %d received data [%f %f %f %f]\n",my_rank,data1[0],data1[1],data1[2],data1[3]);
		    data1=data1+(float4)(1);
		    data2=data2+(float4)(1);
	    }
	    else{
		    data1=i;
		    data2=i+(float4)(1);
        }
        if(my_rank < num_ranks -1){
		    SMI_Push(&chan_send1, &data1);
		    SMI_Push(&chan_send2, &data2);
		     // printf("Rank %d sent data [%f %f %f %f]\n",my_rank,data1[0],data1[1],data1[2],data1[3]);
        }
	    else{
		    check1 &= (all(data1==expected1+(float4)(i)));
		    check2 &= (all(data2==expected2+(float4)(i)));
		    // printf("Last rank received [%f %f %f %f] (exp: [%f %f %f %f])\n",data1[0],data1[1],data1[2],data1[3],expected1[0]+i,expected1[1]+i,expected1[2]+i,expected1[3]+i);
		    // expected1+=(float4)(1);
		    // expected2+=(float4)(1);
	    }
    }
    *mem=check1&check2;
}
