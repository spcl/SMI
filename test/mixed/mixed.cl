/**
    In this test we evaluate the intermixing of p2p
    and collective primitives

    Ranks are organized in a pipeline, in which everyone read
    from the previous increase and sent to next one.

    The last, broadcasts the value
*/
#include <smi.h>
#include "smi_generated_device.cl"
__kernel void test_int(int start, const SMI_Comm comm,__global int *mem)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    SMI_Channel chans=SMI_Open_send_channel(1,SMI_INT,my_rank+1,0,comm);
    SMI_Channel chanr=SMI_Open_receive_channel(1,SMI_INT,my_rank-1,0,comm);
    int data;
    if(my_rank>0)
    {
        SMI_Pop(&chanr, &data);
        data++;
    }
    else
        data=start;

    if(my_rank<num_ranks-1)
        SMI_Push(&chans,&data);


    //at this point a brodcast will happen
    int final=(my_rank==num_ranks-1)?data:0;
    SMI_BChannel  __attribute__((register)) bchan= SMI_Open_bcast_channel(1, SMI_INT,1,num_ranks-1,comm);
    SMI_Bcast(&bchan, &final);
    *mem=final;
}
