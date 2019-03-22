/**
    Example 4 is a MPMD program, characterized by two ranks:
    - rank 0: has two application kernels App_0 and App_2
    - rank 1: has one application kernerls App_1

    The logic is the following:
    - App_0 sends N integers to App_1 through a proper channel
    - App_1 received the N integers and forward them to App_2
    - App_2 receives the data and check that everything is in order


    This program is intended to be executed
    on two FPGAs that have multiple connections, like the node fpga-0014 in Paderborn.
    In FPGA-0014 two QSFPs are used (I/O channels 0 and 3) to connect the two installed FPGAs
    Therefore we will use two pairs CK_S/CK_R for each rank.

    We assume that App_0 and App_1 are attached to the CK_S/CK_R linked to the first
    QSFP (I/O channel 0, the connection with CK_R is needed by App_1 only).
    App_2 is instead attached to the CK_R connected to the other QSFP.

    For the sake of testing (very simple) routes, the routing table of
    CK_Ss in Rank1 imposes that data that goes to rank0 passes through the second
    QSFP (I/O channel 3)

    Many of the interconnections between CKs will be not exploited in the actual execution.
    But we have to place them for compiling the hardware.
*/



#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../../kernels/communications/channel_helpers.h"
#include "../../kernels/communications/push.h"
#include "../../kernels/communications/pop.h"


//#define EMULATION
//#define ON_CHIP
//****This part should be code-generated****

#if defined(EMULATION)
channel SMI_NetworkMessage io_out_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_0_to_1_0")));
channel SMI_NetworkMessage io_out_3 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_0_to_1_3")));
channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_1_to_0_0")));
channel SMI_NetworkMessage io_in_3 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_1_to_0_3")));


#else

channel SMI_NetworkMessage io_out_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch0")));
channel SMI_NetworkMessage io_out_3 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch3")));
channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch0")));
channel SMI_NetworkMessage io_in_3 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch3")));
#endif

//internal routing tables: this will be used by push and pop
//on rank 0, App_0 is connected to CK_S_0
__constant char internal_sender_rt[2]={0,0};
//APP_2 (TAG=1) is instead connected to CK_R_1
//NOTE: in this case we have only one channel between application endpoints
//and CK_Rs. That's why in the table there is a 0
__constant char internal_receiver_rt[2]={0,0};

//channels to CK_S (we use only one of them)
channel SMI_NetworkMessage channels_to_ck_s[2] __attribute__((depth(16)));

//channels from CK_R
channel SMI_NetworkMessage channels_from_ck_r[1] __attribute__((depth(16)));


//inteconnection between CK_S and CK_R (some of these are not used in this example)
__constant char QSFP=2;
channel SMI_NetworkMessage channels_interconnect_ck_s[QSFP*(QSFP-1)] __attribute__((depth(16)));
channel SMI_NetworkMessage channels_interconnect_ck_r[QSFP*(QSFP-1)] __attribute__((depth(16)));
//also we have two channels between each CK_S/CK_R pair
channel SMI_NetworkMessage channels_interconnect_ck_s_to_ck_r[QSFP] __attribute__((depth(16)));
channel SMI_NetworkMessage channels_interconnect_ck_r_to_ck_s[QSFP] __attribute__((depth(16)));




__kernel void CK_sender_0(__global volatile char *restrict rt, const char numRanks)
{
    //in this case the routing table is really simple
    //since from this CK_S we reach RANK 1 through the attached QSFP
    char external_routing_table[256];
    for(int i=0;i<numRanks;i++)
        external_routing_table[i]=rt[i];

    const char ck_id=0;
    /* CK_S accepts input data from other CK_Ss, the paired CK_R, and from Applications.
     * It will read from these input_channel in RR, unsing non blocking read.
     *
     */
    //Number of senders to this CK_S is given by the number of application connected to this CK_S
    //and the number of CK_Ss pair
    const char num_sender=1+1+1; //1 CK_S, 1 CK_R, 1 application
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    //This particular CK_S has App_0 attached
    while(1)
    {
        switch(sender_id)
        {
            case 0: //CK_S
                mess=read_channel_nb_intel(channels_interconnect_ck_s[0], &valid);
            break;
            case 1: //CK_R
                mess=read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[0],&valid);
            break;
            case 2: //appl
                mess=read_channel_nb_intel(channels_to_ck_s[0],&valid);
            break;

        }

        if(valid)
        {
            //check the routing table
            //as possibility we have:
            // - 0, forward to the network (QSFP)
            // - 1, send to the connected CK_R (local send)
            // - 2,... send to other CK_S
            char idx=external_routing_table[GET_HEADER_DST(mess.header)];
            switch(idx)
            {
                case 0:
                    write_channel_intel(io_out_0,mess);
                    break;
                case 1:
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[0],mess);
                    break;
                case 2:
                    write_channel_intel(channels_interconnect_ck_s[1],mess);
                    break;
            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }
}


// THIS CK_S does nothing. But I've implemented it to have all the required connections
__kernel void CK_sender_1(__global volatile char *restrict rt, const char numRanks)
{
    //in this case the routing table is meaningless
    char external_routing_table[256];
    for(int i=0;i<numRanks;i++)
        external_routing_table[i]=rt[i];

    const char ck_id=1;
    /* CK_S accepts input data from other CK_Ss, the paired CK_R, and from Applications.
     * It will read from these input_channel in RR, unsing non blocking read.
     */
    //Number of sender is given by the number of application connected to this CK_S
    //and the number of CK_Ss pair
    const char num_sender=1+1; //1 CK_S, 1 CK_R, 0 application
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    while(1)
    {
        switch(sender_id)
        {
            case 0: //CK_S
                mess=read_channel_nb_intel(channels_interconnect_ck_s[1], &valid);
            break;
            case 1: //CK_R
                mess=read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[1],&valid);
            break;

        }

        if(valid)
        {
            //check the routing table
            //as possibility we have:
            // - 0, forward to the network (QSFP)
            // - 1, send to the connected CK_R (local send)
            // - 2,... send to other CK_S
            char idx=external_routing_table[GET_HEADER_DST(mess.header)];
            switch(idx)
            {
                case 0:
                    write_channel_intel(io_out_3,mess);
                    break;
                case 1:
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[1],mess);
                    break;
                case 2:
                    write_channel_intel(channels_interconnect_ck_s[0],mess);
                    break;
            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }
}


//This one is attached to the first QSFP: it will not do anything
__kernel void CK_receiver_0(__global volatile char *rt,const char myRank, const char numRanks)
{
    //The CK_R will receive from i/o, others CK_R and the paired CK_S (for local sends)

    char external_routing_table[256];
    //load the routing table: in this case is useless
    for(int i=0;i<numRanks;i++)
        external_routing_table[i]=rt[i];

    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    /* The CK_R receives:
     *  - from the QSFP;
     *  -  from other CK_Rs;
     *  - from the paired CK_S;
     */
    const char num_senders=1+1+1; //1 QSFP, 1 CK_R, 1 CK_S
    while(1)
    {
        switch(sender_id)
        {
            case 0: //QSFP
                mess=read_channel_nb_intel(io_in_0,&valid);
            break;
            case 1: //CK_R
                mess=read_channel_nb_intel(channels_interconnect_ck_r[0],&valid);
            break;
            case 2: //paired CK_S
                mess=read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[0],&valid);
            break;
        }
        if(valid)
        {
            //forward it to the right application endpoint or to another CK_R
            //0, send to the connected CK_S (remote send)
            //1, ...,NQ−1N_Q-1N​Q​​−1, send to other CK_R (the application endpoint is not connected to this CK_R)
            //other indexes: send to the application
            char dest;
            if(GET_HEADER_DST(mess.header)!=myRank)
                dest=0;
            else
                dest=external_routing_table[GET_HEADER_TAG(mess.header)];
            //notice: in this case we don't use the internal_receiver_routing_table
            //this is somehow included in the routing table and the code below
            //The internal_receiver_routing_table, will be instead used for generating the external routing_table
            switch(dest)
            {
                case 0:
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[0],mess);
                break;
                case 1:
                    write_channel_intel(channels_interconnect_ck_r[1],mess);
                break;
                    //there no other cases for this, since there is no application endpoints connected to it
            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_senders)
                sender_id=0;

    }

}


__kernel void CK_receiver_1(__global volatile char *rt,const char myRank, const char numRanks)
{
    //The CK_R will receive from i/o, others CK_R and the paired CK_S (for local sends)

    char external_routing_table[256];
    //load the routing table: in this case is useless
    for(int i=0;i<numRanks;i++)
        external_routing_table[i]=rt[i];

    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    /* The CK_R receives:
     *  - from the QSFP;
     *  -  from other CK_Rs;
     *  - from the paired CK_S;
     */
    const char num_senders=1+1+1; //1 QSFP, 1 CK_R, 1 CK_S
    while(1)
    {
        switch(sender_id)
        {
            case 0: //QSFP
                mess=read_channel_nb_intel(io_in_3,&valid);
            break;
            case 1: //CK_R
                mess=read_channel_nb_intel(channels_interconnect_ck_r[1],&valid);
            break;
            case 2: //paired CK_S
                mess=read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[1],&valid);
            break;
        }
        if(valid)
        {
            //forward it to the right application endpoint or to another CK_R
            //0, send to the connected CK_S (remote send)
            //1, ...,NQ−1N_Q-1N​Q​​−1, send to other CK_R (the application endpoint is not connected to this CK_R)
            //other indexes: send to the application
            char dest;
            if(GET_HEADER_DST(mess.header)!=myRank)
                dest=0;
            else
                dest=external_routing_table[GET_HEADER_TAG(mess.header)];
            //printf("Hello, I've received data that goes to %d! The tag was: %d\n",dest,GET_HEADER_TAG(mess.header));
            //notice: in this case we don't use the internal_receiver_routing_table
            //this is somehow included in the routing table and the code below
            //in this case we know that if the routing table there is written 2 then we forward to the application
            switch(dest)
            {
                case 0:
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[1],mess);
                break;
                case 1:
                    write_channel_intel(channels_interconnect_ck_r[0],mess);
                break;
                case 2:
                    write_channel_intel(channels_from_ck_r[0],mess);
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


__kernel void app_0(const int N)
{
    //Rank 1, TAG 0
    SMI_Channel chan=SMI_OpenChannel(0,1,0,N,SMI_INT,SMI_SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        SMI_Push(&chan,&data);
    }
}


__kernel void app_2(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //Please notice: the TAG could have been 0 as well (in app_0 we use tag 0 for sending)
    SMI_Channel chan=SMI_OpenChannel(1,0,1,N,SMI_INT,SMI_RECEIVE);
    for(int i=0; i< N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
       // printf("App received data!!\n");
        check &= (rcvd==expected_0);
        expected_0+=1;
    }
    *mem=check;
}


