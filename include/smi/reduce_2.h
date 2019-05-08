#ifndef REDUCE_2_H
#define REDUCE_2_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "network_message.h"
#include "operation_type.h"

//temp here, then need to move

channel SMI_Network_message channel_reduce_send __attribute__((depth(1)));
channel SMI_Network_message channel_reduce_send_no_root __attribute__((depth(1))); //TODO: tmp, we need to distinguish in the support kernel
channel SMI_Network_message channel_reduce_recv __attribute__((depth(1)));


/*
 * 101 implementation of Reduce with two reduce kernel: it uses two tag to implement this
    Problems:
        - it works with INT only, because the compiler is not able to handle it more "dynamically"
        - problem in the pop part, with the reset of packet_od. For the moment being forced at 7 (it's float, there is no flush)
        - it consumes a lot of resources (dunno)
 *
 */
//align to 64 to remove aliasing
typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;          //buffered network message
    char tag_out;                    //Output channel for the bcast, used by the root
    char tag_in;                     //Input channel for the bcast. These two must be properly code generated. Good luck
    char root_rank;
    char my_rank;                   //These two are essentially the Communicator
    char num_rank;
    uint message_size;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    char packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;               //type of message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
    SMI_Network_message net_2;        //buffered network message
    char packet_element_id_rcv;     //used by the receivers
}SMI_RChannel;


SMI_RChannel SMI_Open_reduce_channel(uint count, SMI_Datatype data_type, char root, char my_rank, char num_ranks)
{
    SMI_RChannel chan;
    //setup channel descriptor
    chan.message_size=count;
    chan.data_type=data_type;
    chan.tag_in=0;
    chan.tag_out=0;
    chan.my_rank=my_rank;
    chan.root_rank=root;
    chan.num_rank=num_ranks;
    switch(data_type)
    {
        case(SMI_INT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=8;
            chan.elements_per_packet=3;
            break;
        case (SMI_CHAR):
            chan.size_of_type=1;
            chan.elements_per_packet=28;
            break;
            //TODO add more data types
    }

    //setup header for the message
    SET_HEADER_DST(chan.net.header,root);
    SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_TAG(chan.net.header,0);        //used by destination TODO: fix properly this tag to avoid confusion
    SET_HEADER_NUM_ELEMS(chan.net.header,0);    //at the beginning no data
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
    return chan;
}



void SMI_Reduce(SMI_RChannel *chan, volatile void* data_snd, volatile void* data_rcv)
{
    //first of all send the data

    char *conv=(char*)data_snd;
    const char chan_idx_out=internal_sender_rt[chan->tag_out];  //This should be properly code generated, good luck
    #pragma unroll
    for(int jj=0;jj<4;jj++) //copy the data
        chan->net.data[chan->packet_element_id*4+jj]=conv[jj];
    //chan->net.data[chan->packet_element_id]=*conv;

    chan->processed_elements++;
    chan->packet_element_id++;
    if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
    {

        SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
        //offload to reduce kernel
        if(chan->my_rank==chan->root_rank)
        {
            write_channel_intel(channel_reduce_send,chan->net);
        }
        else
        {
            write_channel_intel(channel_reduce_send_no_root,chan->net);
        }
        chan->packet_element_id=0;
    }

    if(chan->my_rank==chan->root_rank)//I'm the root, I have to receive from the kernel
    {
        //TODO what is the format of the data that it will send?
        //Still a network message?
        //FOR THE MOMENT BEING THIS IS THE NETWORK MESSAGE

        if(chan->packet_element_id_rcv==0)
        {
            chan->net_2=read_channel_intel(channel_reduce_recv);
        }
        char * ptr=chan->net_2.data+(chan->packet_element_id_rcv)*4;
        //char * ptr=chan->net_2.data+(chan->packet_element_id_rcv);
        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        //TODO: this prevents HyperFlex (try with a constant and you'll see)
        //I had to put this check, because otherwise II goes to 2
        //if we reached the number of elements in this packet get the next one from CK_R
        if(chan->packet_element_id_rcv==7)
            chan->packet_element_id_rcv=0;
        // chan->processed_elements++;                      //TODO: probably useless
        *(int *)data_rcv= *(int*)(ptr);
    }


}

//kernel to be used in case the rank is non-root
__kernel void kernel_reduce_noroot()
{
    SMI_Network_message net_mess,reduce_mess;
    while(true)
    {
        //first of all I have to receive from the root a credit, and then I can
        //send the data to be reduced
        net_mess=read_channel_intel(channels_from_ck_r[1]); //tag 1
        printf("[NO ROOT] Received token, send data\n");
        //this must be a credit
        reduce_mess=read_channel_intel(channel_reduce_send_no_root);
        write_channel_intel(channels_to_ck_s[1],reduce_mess); //tag 1
    }

}
//temp here, then if it works we need to move it
//TODO: this one has to know if the current rank is the root
//it is probably a bad idea to fix it as parameter, because this must be started from the host
//FOR THE MOMENT BEING: let's assume that if it receive something from a ck_s then this rank is
//the root. So this one will receive from the application and/or CK_R, and send to application
//and/or CK_S. OR ALSO we can look at the message_sender_receiver
//TODO: where is the operation????? We need this as a parameter somewhere
//Possible solutions: use the tag, or you need to have another internal data type
//FOR THE MOMENT BEING: it is just the add
//TODO: decide data format exchanged with the application

//TODO: what about the data type??? who tells you that? Codegen?
//TODO: how many "multiple channel" shall we use, for the moment two toward ck_s and one from ck_r
__kernel void kernel_reduce_root(const char num_rank)
{
    //Version with credit flow control
    //buffer size  =  1 for the moment
    //TODO decide wheter we want to have a network message or the data element directly as buffer

    SMI_Network_message mess,mess_root;
    SMI_Network_message reduce; //TODO: decide the internal data format
    bool init=true;
    char sender_id=0;
    const char credits_flow_control=7;
    const char credits_flow_control_minus_one=credits_flow_control-1;
    int reduce_result[credits_flow_control];
    char data_recvd[credits_flow_control];
    bool send_credits=false;//true if (the root) has to send reduce request
    char credits=credits_flow_control; //the number of credits that I have
    char send_to=0;
    char __attribute__((register)) add_to[16];   //for each rank tells to what element in the buffer we should add the received item
    for(int i=0;i<credits_flow_control;i++)
        data_recvd[i]=0;
    for(int i=0;i<16;i++)
       add_to[i]=0;
    char current_buffer_element=0;
    char add_to_root=0;
    while(true)
    {
        SMI_Network_message mess,mess_root;
        bool valid=false;
        if(!send_credits)
        {
#if 0
            mess_root=read_channel_nb_intel(channel_reduce_send,&valid);
            if(valid)
            {
                char * ptr=mess_root.data;
                int data= *(int*)(ptr);
                reduce_result[add_to_root]+=data;        //SMI_ADD
                //printf("Reduce kernel received from app, root. Adding it to: %d \n",add_to_root);
                data_recvd[add_to_root]++;
                send_credits=init;      //the first reduce, we send this
                init=false;
                add_to_root++;
                if(add_to_root==credits_flow_control)
                    add_to_root=0;
            }
            mess=read_channel_nb_intel(channels_from_ck_r[0],&valid);
            if(valid)
            {
                char rank=GET_HEADER_SRC(mess.header);
                char addto=add_to[rank];
                char tmp=addto;
                if(addto==credits_flow_control_minus_one)
                    addto=0;
                else
                    addto++;
                add_to[rank]=addto;
                char * ptr=mess.data;
                int data= *(int*)(ptr);
                data_recvd[tmp]++;
                reduce_result[tmp]+=data;        //SMI_ADD
            }
#endif
//#if 0
            switch(sender_id)
            {   //for the root, I have to receive from both sides
                case 0:
                    mess_root=read_channel_nb_intel(channel_reduce_send,&valid);
                    break;
                case 1: //read from CK_R, can be done by the root and by the non-root
                    mess=read_channel_nb_intel(channels_from_ck_r[/*chan_idx*/0],&valid);
                    break;
            }
            if(valid)
            {
                if(sender_id==0)
                {
                    //the data from the root. Adds it to the reduced result
                    char * ptr=mess_root.data;
                    int data= *(int*)(ptr);
                    reduce_result[add_to_root]+=data;        //SMI_ADD
                    printf("Reduce kernel received from app, root. Adding it to: %d \n",add_to_root);
                    data_recvd[add_to_root]++;
                    send_credits=init;      //the first reduce, we send this
                    init=false;
                    add_to_root++;
                    if(add_to_root==credits_flow_control)
                        add_to_root=0;
                }
                else
                {
                    //received from CK_R, another rank
                    //apply reduce
                    char rank=GET_HEADER_SRC(mess.header);
                    char addto=add_to[rank];
                    char tmp=addto;
                    if(addto==credits_flow_control_minus_one)
                        addto=0;
                    else
                        addto++;
                    add_to[rank]=addto;
                    char * ptr=mess.data;
                    int data= *(int*)(ptr);
                    printf("[ROOT] Received from %d data: %d\n",rank,data);
                    data_recvd[tmp]++;
                    reduce_result[tmp]+=data;        //SMI_ADD
                    //addto++;
                    //add_to[rank]++;
                }
                //printf("a: %d, data_recvd: %d\n",(int)a,(int)data_recvd[a]);
                //data_recvd[a]++;
                //printf("reduce result %d, received %d out of %d\n",(int)a,(int)data_recvd[a],num_rank);
            }
//#endif
            if(data_recvd[current_buffer_element]==num_rank) //TODO: evitare di fare questa cosa su tutti i rank!
            {
                SMI_Network_message reduce; //TODO: decide the internal data format
                //printf("Reduce kernel, send to app: %d\n",reduce_result[current_buffer_element]);
                //send to application
                char *data_snd=reduce.data;
                char *conv=(char*)(&reduce_result[current_buffer_element]);
                #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    data_snd[jj]=conv[jj];
                write_channel_intel(channel_reduce_recv,reduce);
                send_credits=true;
                credits++;
                data_recvd[current_buffer_element]=0;
                reduce_result[current_buffer_element]=0;
                current_buffer_element++;
                if(current_buffer_element==credits_flow_control)
                    current_buffer_element=0;

            }
            if(sender_id==0)
                sender_id=1;
            else
                sender_id=0;
        }
        else
        {
            //send credits
            //char st=send_to;

            if(send_to!=GET_HEADER_DST(mess.header))
            {
                SET_HEADER_OP(reduce.header,SMI_REQUEST);
                SET_HEADER_TAG(reduce.header,1); //TODO: Fix this tag
                SET_HEADER_DST(reduce.header,send_to);
                printf("[REDUCE] send credit to %d\n",send_to);
                write_channel_intel(channels_to_ck_s[0],reduce);

            }
            send_to++;
            if(send_to==num_rank)
            {
                send_to=0;
                credits--;
                send_credits=(credits!=0);
            }

        }
        //mem_fence(CLK_CHANNEL_MEM_FENCE);

    }

}

#endif // REDUCE_2_H
