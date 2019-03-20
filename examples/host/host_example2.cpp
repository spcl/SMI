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
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'f':
                fpga=atoi(optarg);
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

    if(sender)
    {
        cout << "Starting the sender on FPGA "<<fpga<<endl;
        kernel_names.push_back("app_sender");
        kernel_names.push_back("CK_sender_0");
        kernel_names.push_back("CK_sender_1");
    }
    else
    {
        cout << "Starting a receiver on FPGA "<<fpga<<endl;
        kernel_names.push_back("app_receiver");
        kernel_names.push_back("CK_receiver");

    }
    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char res;
    //ATTENTION: declare all the memory here otherwise it hangs
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer routing_table1(context,CL_MEM_WRITE_ONLY,3);
    cl::Buffer routing_table2(context,CL_MEM_WRITE_ONLY,3);

    if(sender)
    {
        char ranks=3;
        char rt1[3]={0,0,1};
        char rt2[3]={100,100,0};
        queues[0].enqueueWriteBuffer(routing_table1, CL_TRUE,0,3,rt1);
        queues[0].enqueueWriteBuffer(routing_table2, CL_TRUE,0,3,rt2);
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(cl_mem),&routing_table1);
        kernels[1].setArg(1,sizeof(char),&ranks);
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table2);
        kernels[2].setArg(1,sizeof(char),&ranks);
    }
    else
    {
        kernels[0].setArg(0,sizeof(cl_mem),&check);
        kernels[0].setArg(1,sizeof(int),&n);
    }


    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size();i++)
        queues[i].enqueueTask(kernels[i]);


    queues[0].finish();

    timestamp_t end=current_time_usecs();

    std::cout << "Sleeping a bit to ensure CK have finished their job"<<std::endl;
    sleep(1);
    if(receiver)
    {
        queues[0].enqueueReadBuffer(check,CL_TRUE,0,1,&res);
        if(res==1)
            cout << "OK!"<<endl;
        else
            cout << "Error!!"<<endl;

        cout << "Time elapsed (usecs): "<<end-start<<endl;
    }

}
