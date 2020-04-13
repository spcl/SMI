/**
    Balanced routing tests. Evaluation over  different communication patterns:
    - test_pipeline: p2p communication, each rank has two channels in and two channels out. It receives from the
        previous rank (if any) and sends to the next one (if any)
*/
#include <smi.h>

__kernel void test_pipeline(__global char* mem, const int N, SMI_Comm comm)
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
        }
	    else{
	        check1 &= (data1==expected1);
		    check2 &= (data2==expected2);
		    expected1++;
		    expected2++;
	    }
    }
    *mem=check1&check2;
}


__kernel void test_broadcast(__global char* mem, const int N, char root, SMI_Comm comm)
{
    char check=1;
    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    SMI_BChannel  __attribute__((register)) chan1= SMI_Open_bcast_channel(N, SMI_INT,2, root,comm);
    SMI_BChannel  __attribute__((register)) chan2= SMI_Open_bcast_channel(N, SMI_INT,3, root,comm);

    for(int i=0;i<N;i++)
    {

        int to_comm=i;
        int to_comm2=i;
        SMI_Bcast(&chan1,&to_comm);
        SMI_Bcast(&chan2,&to_comm2);
        check &= (to_comm==i && to_comm2==i);
    }
    *mem=check;
}