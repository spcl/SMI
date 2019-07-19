/*
	Injection rate benchmark. Rank 0 sends data using a
	transient channel with message size 1 to a receiving rank
	whose rank id is specied as parameter
*/


#include <smi.h>



__kernel void app(const int N, const char dst, SMI_Comm comm)
{
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,dst,0,comm);
        SMI_Push(&chan_send,&i);
    }
}
