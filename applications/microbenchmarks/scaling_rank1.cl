/**
    Scaling benchmark: we want to evaluate the bandwdith
    achieved between two ranks. The FPGA are connected in a chain
    so we can decide the distance at which they are

    RANK 0 is the source of the data
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "codegen_scaling/smi-device-1.h"

__kernel void app(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_DOUBLE,0,0,comm);
    const double start=0.1f;
    char check=1;
    for(int i=0;i<N;i++)
    {

        double rcvd;
        SMI_Pop(&chan,&rcvd);
        //printf("[RCV 0] ricevuto %d\n",i);
//        if(rcvd!=(start+i))
  //          printf("Error in receiving...\n");
        check &= (rcvd==(start+i));
    }
    *mem=check;
  //  printf("App 0 finished\n");

}

__kernel void app_1(__global char *mem, const int N,SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_DOUBLE,0,1,comm);
    const double start=0.1f;
    char check=1;

    for(int i=0;i<N;i++)
    {

        double rcvd;
        SMI_Pop(&chan,&rcvd);
        //printf("[RCV 1] ricevuto %d\n",i);

//        if(rcvd!=(start+i))
  //          printf("Error in receiving...\n");
        check &= (rcvd==(start+i));
    }
    *mem=check;
  //  printf("App 1 finished\n");

}
