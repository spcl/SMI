/**
    In this example we have 1 sender and two receivers.
    Sender is allocated on FPGA0, receivers are allocate on FPGA1

            FPGA0   -------> FPGA1

    On FPGA1, receivers are allocated on two different CK_Rs.
    CK_R_0 is responsible for receiving data from the QSFP0 (which is where the
    connection with FPGA0 is linked) and is connected to APP1 and CK_R_1 (in/out).

    CK_R_1 is responsible for receiving data from the QSFP1 (no attached connection)
    CK_R_0 and is connected to APP2.

    Therefore messages directed to APP2 are received from CK_R_0 and eventually routed to CK_R_1

*/



#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../../kernels/communications/channel_helpers.h"
#include "../../kernels/communications/pop.h"

//****This part should be code-generated****

//in this case we use also another i/o input channel for CK_R_1
//ven if it is not used

#if defined(EMULATION)
channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_0")));
channel SMI_NetworkMessage io_in_1 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_1")));
#else

channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch1")));
channel SMI_NetworkMessage io_in_1 __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch2")));

#endif
//internal routing tables
__constant char internal_receiver_rt[2]={0,1};

//inteconnection between CK_R (one of this two is actually never used)
channel SMI_NetworkMessage channel_interconnect_ck_r[2] __attribute__((depth(16)));


//channel from CK_R
channel SMI_NetworkMessage channel_from_ck_r[2] __attribute__((depth(16)));



__kernel void CK_receiver_0(__global volatile char *rt,const char numRanks)
{
    //The CK_R will receive from i/o, others CK_R (and possibly a CK_S)
    //In this case, there will be no packet to RANK 0
    //And for Rank 1 (this rank)
    //Also, we statically know how many TAG we have in this program
    //(in general we know the max number is 256, but having all of them will use a lot of BRAMs)
    const char numTAGs=2;
    char external_routing_table[32][numTAGs];
    for(int i=0;i<numRanks;i++)
    {

        for(int j=0;j<numTAGs;j++)
            external_routing_table[i][j]=rt[i*numTAGs+j];
    }
    //char external_routing_table[2][2]={{100,100},{0,1}};

    const char myRank=1;
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    const char num_senders=1+1; //1 is the QSFP and the other is CK_R
    while(1)
    {
        switch(sender_id)
        {
            case 0:
                mess=read_channel_nb_intel(io_in_0,&valid);
            break;
            case 1:
                mess=read_channel_nb_intel(channel_interconnect_ck_r[1],&valid);
            break;
        }
        if(valid)
        {
            //forward it to the right application endpoint or to another CK_R
            char dest=external_routing_table[GET_HEADER_DST(mess.header)][GET_HEADER_TAG(mess.header)];
            //in this case
            //dest = 0 -> application endpoint
            //dest = 1 -> the other CK_R
            //notice: in this case we don't use the internal_receiver_routing_table
            //this is somehow included in the routing table and the code below
            switch(dest)
            {
                case 0:
                    write_channel_intel(channel_from_ck_r[0],mess);
                break;
                case 1:
                    write_channel_intel(channel_interconnect_ck_r[0],mess);
                break;
            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_senders)
                sender_id=0;

    }

}


__kernel void CK_receiver_1(__global volatile char *rt, const char numRanks)
{
    //The CK_R will receive from i/o, others CK_R (and possibly a CK_S)
    //In this case, there will be no packet to RANK 0
    //And for Rank 1 (this rank)

    const char numTAGs=2;
     char  external_routing_table[32][numTAGs];
    for(int i=0;i<numRanks;i++)
    {
        //Attention: if the routing_table < 1024 byte it will be stored as register
        //and it is better to unroll this loop (usually the compiler is smart enough)
        //#pragma unroll  1
        for(int j=0;j<numTAGs;j++)
            external_routing_table[i][j]=rt[i*numTAGs+j];
    }
    //char external_routing_table[2][2]={{100,100},{1,0}};
    const char myRank=1;
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    const char num_senders=1+1; //1 is the QSFP and the other is CK_R
    while(1)
    {
        switch(sender_id)
        {
            case 0:
                mess=read_channel_nb_intel(io_in_1,&valid); //THIS read will always fail
            break;
            case 1:
                mess=read_channel_nb_intel(channel_interconnect_ck_r[0],&valid);
            break;
        }
        if(valid)
        {
            //forward it to the right application endpoint or to another CK_R
            char dest=external_routing_table[GET_HEADER_DST(mess.header)][GET_HEADER_TAG(mess.header)];
            //in this case
            //dest = 0 -> application endpoint
            //dest = 1 -> the other CK_R
            //notice: in this case we don't use the internal_receiver_routing_table
            //this is somehow included in the routing table and the code below
            switch(dest)
            {
                case 0:
                    write_channel_intel(channel_from_ck_r[1],mess);
                break;
                case 1:
                    write_channel_intel(channel_interconnect_ck_r[1],mess);
                break;
            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_senders)
                sender_id=0;

    }

}

//****End code-generated part ****


__kernel void app_receiver_1(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    SMI_Channel chan=SMI_OpenChannel(1,0,0,N,SMI_INT,SMI_RECEIVE);
    bool immediate=false;
    for(int i=0; i< N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==expected_0);
        expected_0+=1;
    }

    *mem=check;
}


__kernel void app_receiver_2(__global volatile char *mem, const int N)
{
    char check=1;
    float expected_0=1.1f;
    SMI_Channel chan=SMI_OpenChannel(1,0,1,N,SMI_FLOAT,SMI_RECEIVE);

    for(int i=0; i< N;i++)
    {
        float rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected_0+i));
    }

    *mem=check;
}

