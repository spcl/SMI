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
    if(argc<9)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -t <type s/r> -f <fpga> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    bool receiver=true;
    bool sender=true;
    int fpga;
    while ((c = getopt (argc, argv, "n:b:t:f:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'f':
                fpga=atoi(optarg);
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
        kernel_names.push_back("sender");
    }
    else
    {
        cout << "Starting the receiver on FPGA "<<fpga<<endl;
        kernel_names.push_back("receiver");

    }

    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);
    char res_1;
    //create memory buffers
    cl::Buffer check_1(context,CL_MEM_WRITE_ONLY,1);
    if(sender)
    {
        kernels[0].setArg(0,sizeof(int),&n);
    }
    else
    {
        kernels[0].setArg(0,sizeof(cl_mem),&check_1);
        kernels[0].setArg(1,sizeof(int),&n);
    }

    if(sender)
        std::cout << "Let's start the sender" <<std::endl;
    else
        std::cout << "Let's start the receiver" <<std::endl;

    timestamp_t start=current_time_usecs();
    queues[0].enqueueTask(kernels[0]);
    queues[0].flush();
    if(sender)
        std::cout << "Let's wait the sender " <<std::endl;
    else
        std::cout << "Let's wait the receiver " <<std::endl;

    queues[0].finish();
    timestamp_t end=current_time_usecs();


    if(receiver)
    {
        queues[0].enqueueReadBuffer(check_1,CL_TRUE,0,1,&res_1);
        if(res_1==1)
            cout << "OK!"<<endl;
        else
            cout << "Error!!"<<endl;
        cout << "Time elapsed (usecs): "<<end-start<<endl;
    }



}
