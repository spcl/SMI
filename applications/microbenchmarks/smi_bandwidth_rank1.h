#pragma OPENCL EXTENSION cl_intel_channels : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "../../kernels/communications/channel_helpers.h"



//#define EMULATION
//****This part should be code-generated****

#if defined(EMULATION)
channel SMI_Network_message io_out_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel1")));
channel SMI_Network_message io_out_1 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_15_1_to_15_0_1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_tmp")));
channel SMI_Network_message io_out_3 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_tmp")));

channel SMI_Network_message io_in_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel")));
channel SMI_Network_message io_in_1 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_15_0_to_15_1_1")));
channel SMI_Network_message io_in_2 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_tmp")));
channel SMI_Network_message io_in_3 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_tmp")));

#else

channel SMI_Network_message io_out_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch3")));
channel SMI_Network_message io_in_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch1")));
channel SMI_Network_message io_in_2 __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch3")));
#endif



//on rank 1 we have 3 applications
__constant char internal_receiver_rt[3]={0,1,2};

//channels to CK_S  (not used here)
//channel SMI_Network_message channels_to_ck_s[2] __attribute__((depth(16)));

//channels from CK_R
channel SMI_Network_message channels_from_ck_r[3] __attribute__((depth(16)));


//inteconnection between CK_S and CK_R (some of these are not used in this example)
__constant char num_qsfp=4;
channel SMI_Network_message channels_interconnect_ck_s[num_qsfp*(num_qsfp-1)] __attribute__((depth(16)));
channel SMI_Network_message channels_interconnect_ck_r[num_qsfp*(num_qsfp-1)] __attribute__((depth(16)));
//also we have two channels between each CK_S/CK_R pair
channel SMI_Network_message channels_interconnect_ck_s_to_ck_r[num_qsfp] __attribute__((depth(16)));
channel SMI_Network_message channels_interconnect_ck_r_to_ck_s[num_qsfp] __attribute__((depth(16)));


void SMI_Pop(SMI_Channel *chan, void *data)
{
    //in this case we have to copy the data into the target variable
    if(chan->packet_element_id==0)
    {
        const char chan_idx=internal_receiver_rt[chan->tag];
        chan->net=read_channel_intel(channels_from_ck_r[chan_idx]);
    }
    char * ptr=chan->net.data+(chan->packet_element_id)*chan->size_of_type;
    chan->packet_element_id++;                       //first increment and then use it: otherwise compiler detects Fmax problems
    //TODO: this prevents HyperFlex (try with a constant and you'll see)
    //I had to put this check, because otherwise II goes to 2
    if(chan->packet_element_id==GET_HEADER_NUM_ELEMS(chan->net.header))
        chan->packet_element_id=0;
    //if we reached the number of elements in this packet get the next one from CK_R
    chan->processed_elements++;                      //TODO: probably useless
    //create data element
    if(chan->data_type==SMI_INT)
        *(int *)data= *(int*)(ptr);
    if(chan->data_type==SMI_FLOAT)
        *(float *)data= *(float*)(ptr);
    if(chan->data_type==SMI_DOUBLE)
        *(double *)data= *(double*)(ptr);

}
