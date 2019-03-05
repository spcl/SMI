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
    if(argc<5)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    bool receiver=true;
    bool sender=true;
    while ((c = getopt (argc, argv, "n:b:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
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
    std::vector<std::string> kernel_names={"app_sender_1","app_sender_2","app_receiver_1","app_receiver_2","packer_1","packer_2","unpacker_1",
                        "unpacker_2","CK_sender","CK_receiver"};

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,context,program,program_path,kernel_names, kernels,queues);

    cout << "Starting the receiver..."<<endl;
    //create memory buffers
    char res_1,res_2;
    cl::Buffer check_1(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer check_2(context,CL_MEM_WRITE_ONLY,1);
    kernels[0].setArg(0,sizeof(int),&n);
    kernels[1].setArg(0,sizeof(int),&n);
    kernels[2].setArg(0,sizeof(cl_mem),&check_1);
    kernels[2].setArg(1,sizeof(int),&n);
    kernels[3].setArg(0,sizeof(cl_mem),&check_2);
    kernels[3].setArg(1,sizeof(int),&n);


    cout << "Starting the sender..."<<endl;

    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size();i++)
        queues[i].enqueueTask(kernels[i]);

    for(int i=0;i<4;i++)
        queues[i].finish();

    timestamp_t end=current_time_usecs();
    cout << "Time elapsed (usecs): "<<end-start<<endl;


    queues[0].enqueueReadBuffer(check_1,CL_TRUE,0,1,&res_1);
    queues[0].enqueueReadBuffer(check_2,CL_TRUE,0,1,&res_2);
    if(res_1==1 && res_2==1)
        cout << "OK!"<<endl;
    else
        cout << "Error!!"<<endl;



}
