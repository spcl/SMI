/**
 	Example 5 is a MPMD program, characterized by three ranks:
    - rank 0: has two application kernels App_0 and App_1
    - rank 1: has one application kernel App_2
    - rank 3: has one application kernel App_3

    The logic is the following:
    - App_0 sends N integers to App_2 through a proper channel (TAG 0)
    - App_1 sends N floats to App_3 through a proper channel (TAG 1)

	The program will be execute on three FPGA nodes ( node 15 acl0, 15:1 and 16:1)
	using the following connections:
	- 15:0:chan1 - 15:1:chan1
	- 15:1:chan0 - 16:1:chan0

	So we don't use the crossbar. This is done to see if everything works with an intermediate hop

	Also, considering that for the moment being we use only 2 QSFPs and numbering the CK_S with 0 and 1
	we impose that:
	- APP 0 is connected to CK_S_0: in this way its sent data must go to CK_S_1 and then to QSFP
	- APP 1 is connected to CK_S_1: so data will go through QSFP1, will be received to CK_R_1 on rank 1
		which will route it to its paired CK_S_1, who will route it to CK_S_1 (in rank 1) who finally
		sends it to rank 2
	- APP_2 is connected to CK_R_1 (so no additional hop)
	- APP_3 is connected to CK_R_0 (so no additional hop)

	Therefore the routing table for CK_Ss will be: (remember 0-> QSFP, 1 CK_R, 2... connected CK_S. This maps ranks-> ids)
	- rank 0: 	CK_S_0: 1,2,2
				CK_S_1: 1,0,0
	- rank 1:	CK_S_0: 2,1,0
				CK_S_1: 0,1,2
	- rank 2: meaningless

	The routing tables for CK_Rs, instead, maps TAGs to internal port id (0,...Q-2 other CK_R, others: application endpoints):
	- rank 0: meaningless
	- rank 1: 	CK_R_0: meaningless (no attached endpoints), if it receives something with TAG 0 it should send to CK_R_1 but we are not using it
				CK_R_1: 2,xxx (send to APP_2...tag 1 is already handled by looking at the rank and forwarding to the CKS)
	- rank 2: 	CK_R_0: xxx, 2


 */



#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../../kernels/communications/channel_helpers.h"
//#include "../../kernels/communications/push.h"
#include "../../kernels/communications/pop.h"


//#define EMULATION
//#define ON_CHIP
//****This part should be code-generated****

#if defined(EMULATION)
channel SMI_NetworkMessage io_out_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_16_1_to_15_1_0")));
channel SMI_NetworkMessage io_out_1 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_16_1_to_16_0_1")));
channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_15_1_to_16_1_0")));
channel SMI_NetworkMessage io_in_1 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_16_0_to_16_1_1")));

#else

channel SMI_NetworkMessage io_out_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch0")));
channel SMI_NetworkMessage io_out_1 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch1")));
channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch0")));
channel SMI_NetworkMessage io_in_1 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch1")));
#endif



//internal routing tables: this will be used by push and pop
//on rank 1, APPs do not send, so we acoid including the push.h
//__constant char internal_sender_rt[2]={0,1};
//in rank 1, APP_2 (tag 0) is connected to CK_R_1
__constant char internal_receiver_rt[2]={100,0};

//channels to CK_S  (not used here)
//channel SMI_NetworkMessage channels_to_ck_s[2] __attribute__((depth(16)));

//channels from CK_R
channel SMI_NetworkMessage channels_from_ck_r[1] __attribute__((depth(16)));


//inteconnection between CK_S and CK_R (some of these are not used in this example)
__constant char QSFP=2;
channel SMI_NetworkMessage channels_interconnect_ck_s[QSFP*(QSFP-1)] __attribute__((depth(16)));
channel SMI_NetworkMessage channels_interconnect_ck_r[QSFP*(QSFP-1)] __attribute__((depth(16)));
//also we have two channels between each CK_S/CK_R pair
channel SMI_NetworkMessage channels_interconnect_ck_s_to_ck_r[QSFP] __attribute__((depth(16)));
channel SMI_NetworkMessage channels_interconnect_ck_r_to_ck_s[QSFP] __attribute__((depth(16)));



//This one does not anything
__kernel void CK_sender_0(__global volatile char *restrict rt, const char numRanks)
{
    char external_routing_table[256];
    for(int i=0;i<numRanks;i++)
        external_routing_table[i]=rt[i];

    const char ck_id=0;
    /* CK_S accepts input data from other CK_Ss, the paired CK_R, and from Applications.
     * It will read from these input_channel in RR, unsing non blocking read.
     */
    //Number of senders to this CK_S is given by the number of application connected to this CK_S
    //and the number of CK_Ss pair
    const char num_sender=1+1; //1 CK_S, 1 CK_R
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


//this one does not receives from application
__kernel void CK_sender_1(__global volatile char *restrict rt, const char numRanks)
{
    
    char external_routing_table[256];
    for(int i=0;i<numRanks;i++)
        external_routing_table[i]=rt[i];

    const char ck_id=1;
    /* CK_S accepts input data from other CK_Ss, the paired CK_R, and from Applications.
     * It will read from these input_channel in RR, unsing non blocking read.
     */
    //Number of sender is given by the number of application connected to this CK_S
    //and the number of CK_Ss pair
    const char num_sender=1+1; //1 CK_S, 1 CK_R
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
                    write_channel_intel(io_out_1,mess);
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


//This one is attached to the first QSFP: it will receive the data for APP 3
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
                case 2:
                    write_channel_intel(channels_from_ck_r[0],mess);
                    break;               }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_senders)
                sender_id=0;

    }

}

//This one do nothing
__kernel void CK_receiver_1(__global volatile char *rt,const char myRank, const char numRanks)
{
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
                mess=read_channel_nb_intel(io_in_1,&valid);
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
                                
            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_senders)
                sender_id=0;

    }

}




//****End code-generated part ****
__kernel void app_3(__global volatile char *mem, const int N)
{
    char check=1;
    float expected_0=1.1f;
    SMI_Channel chan=SMI_OpenReceiveChannel(N,SMI_FLOAT,0,1);
    for(int i=0; i< N;i++)
    {
        float rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected_0+i));
    }

    *mem=check;
}

