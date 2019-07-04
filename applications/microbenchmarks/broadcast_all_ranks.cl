#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "broadcast/smi-device-0.h"



__kernel void app(__global char* mem, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    SMI_Channel chans=SMI_Open_send_channel(N,SMI_INT,1,0);
    SMI_Channel chanr=SMI_Open_receive_channel(N,SMI_INT,0,0);
    int data=N;
    SMI_Push(&chans,&data);
    SMI_Pop(&chanr,&data);

    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_FLOAT,1, root,my_rank,num_ranks);
    for(int i=0;i<data;i++)
    {

        int to_comm=i;//count_updated[i];
        SMI_Bcast(&chan,&to_comm);

           check &= (to_comm==i);
    }

    *mem=check;

}
