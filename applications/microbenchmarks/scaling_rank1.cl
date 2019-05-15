/**
    Scaling benchmark: we want to evaluate the bandwdith
    achieved between two ranks. The FPGA are connected in a chain
    so we can decide the distance at which they are

    RANK 0 is the source of the data
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "scaling_benchmark/smi_rank1.h"

__kernel void app(__global char *mem, const int N)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_DOUBLE,0,0);
    const double start=0.1f;
    char check=1;
    const int outer_loop_limit=N/3;
    for(int i=0;i<outer_loop_limit;i++)
    {
        for(int j=0;j<3;j++)
        {
            double rcvd;
            SMI_Pop(&chan,&rcvd);
    //        if(rcvd!=(start+i))
      //          printf("Error in receiving...\n");
            check &= (rcvd==(start+i*3+j));
        }
    }
    *mem=check;

}
