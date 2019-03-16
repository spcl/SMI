/**
 * Host program for basic_infrastructure.
 * Everything is on the same FPGA, so the host program launches all the kernels
 */
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../include/utils/ocl_utils.hpp"
#include "../include/utils/utils.hpp"

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    //command line argument parsing
    if(argc<11)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -t <type s/r> -f <fpga> -s <num_senders 2/3>"<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    bool receiver=true;
    bool sender=true;
    int fpga;
    int num_senders;
    while ((c = getopt (argc, argv, "n:b:t:f:s:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'f':
                fpga=atoi(optarg);
                break;
            case 's':
                num_senders=atoi(optarg);
                if(num_senders!=2 && num_senders!=3)
                {
                    std::cerr << "Num senders must be 2 or 3" << std::endl;
                    exit(-1);
                }
            break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 't':
                {
                char t=*optarg;
                if(t=='s')
                {
                    receiver=false;
                    sender=true;
                }
                else
                    if(t=='r')
                    {
                        receiver=true;
                        sender=false;
                    }
                else
                    {
                        cerr << "Error: type may be s (sender) or r (receiver)"<<endl;
                        exit(-1);
                    }

                break;
                }
            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cout << "Performing send/receive test with "<<n<<" integers"<<endl;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    //this is for the case with classi channels
    if(sender)
    {
        cout << "Starting the sender on FPGA "<<fpga<<endl;
        kernel_names.push_back("app_sender_1");
        kernel_names.push_back("app_sender_2");
        if(num_senders==3)
            kernel_names.push_back("app_sender_3");
        kernel_names.push_back("CK_sender");
    }
    else
    {
        cout << "Starting the receiver on FPGA "<<fpga<<endl;
        kernel_names.push_back("app_receiver_1");
        kernel_names.push_back("app_receiver_2");
        if(num_senders==3)
            kernel_names.push_back("app_receiver_3");
        kernel_names.push_back("CK_receiver");

    }

    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);
    char res_1,res_2,res_3;
    //create memory buffers
    cl::Buffer check_1(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer check_2(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer check_3(context,CL_MEM_WRITE_ONLY,1);

    if(sender)
    {
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(int),&n);
        if(num_senders==3)
            kernels[2].setArg(0,sizeof(int),&n);
    }
    else
    {
        kernels[0].setArg(0,sizeof(cl_mem),&check_1);
        kernels[0].setArg(1,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(cl_mem),&check_2);
        kernels[1].setArg(1,sizeof(int),&n);
        if(num_senders==3)
        {
            kernels[2].setArg(0,sizeof(cl_mem),&check_3);
            kernels[2].setArg(1,sizeof(int),&n);
        }
    }


    std::cout << "Let's start " <<std::endl;
    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size();i++)
        queues[i].enqueueTask(kernels[i]);

    for(int i=0;i<num_senders;i++)
    {
        queues[i].finish();
        cout << "Kernel "<<i<<" finished"<<endl;
    }
    timestamp_t end=current_time_usecs();
    std::cout << "Sleeping a bit to ensure CK have finished their job"<<std::endl;
    sleep(1);

    if(receiver)
    {
        queues[0].enqueueReadBuffer(check_1,CL_TRUE,0,1,&res_1);
        queues[0].enqueueReadBuffer(check_2,CL_TRUE,0,1,&res_2);
        if(num_senders==3)
            queues[0].enqueueReadBuffer(check_3,CL_TRUE,0,1,&res_3);
        else
            res_3=1;


        if(res_1==1 && res_2==1  && res_3==1)
            cout << "OK!"<<endl;
        else
            cout << "Error!!"<<endl;
        cout << "Time elapsed (usecs): "<<end-start<<endl;
    }



}
