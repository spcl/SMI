#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "broadcast/smi-device-0.h"



__kernel void app(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;


    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,0, root,comm);
    for(int i=0;i<N;i++)
    {

        int to_comm=i;//count_updated[i];
        SMI_Bcast(&chan,&to_comm);
        if(to_comm!=i)
            printf("Rank %d, received %d instead of %d\n",SMI_Comm_rank(comm),to_comm,i);
        check &= (to_comm==i);
    }

    *mem=check;

}
