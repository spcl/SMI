/**
 * Host program for example 4.
 */
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <rank 0/1> -f <fpga> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char rank;
    int fpga;
    while ((c = getopt (argc, argv, "n:b:r:f:")) != -1)
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
            case 'r':
                {
                    rank=atoi(optarg);
                    if(rank!=0 && rank!=1)
                    {
                        cerr << "Error: rank may be 1 or 0"<<endl;
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

    if(rank==0)
    {
        cout << "Starting rank 0 on FPGA "<<fpga<<endl;
        kernel_names.push_back("app_0");
        kernel_names.push_back("app_2");
        kernel_names.push_back("CK_sender_0");
        kernel_names.push_back("CK_sender_1");
        kernel_names.push_back("CK_receiver_0");
        kernel_names.push_back("CK_receiver_1");

    }
    else
    {
        cout << "Starting rank 1 on FPGA "<<fpga<<endl;
        kernel_names.push_back("app_1");
        kernel_names.push_back("CK_sender_0");
        kernel_names.push_back("CK_sender_1");
        kernel_names.push_back("CK_receiver_0");
        kernel_names.push_back("CK_receiver_1");

    }
    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char res;
    //ATTENTION: declare all the memory here otherwise it hangs
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);


    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,2);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,2);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,2);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,2);
    if(rank==0)
    {
        char ranks=2;
        //senders routing tables
        char rt_s_0[2]={1,0}; //from CK_S_0 we go to the QSFP
        char rt_s_1[2]={1,2}; //from CK_S_1 we go to CK_S_0 to go to rank 1 (not used)
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,2,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,2,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={100,100}; //the first doesn't do anything
        char rt_r_1[2]={0,2};   //the second is attached to the ck_r (channel has TAG 1)
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,2,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,2,rt_r_1);
        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(cl_mem),&check);
        kernels[1].setArg(1,sizeof(int),&n);
        //args for the CK_Ss
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[2].setArg(1,sizeof(char),&ranks);
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[3].setArg(1,sizeof(char),&ranks);

        //args for the CK_Rs
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[4].setArg(1,sizeof(char),&rank);
        kernels[4].setArg(2,sizeof(char),&ranks);
        kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[5].setArg(1,sizeof(char),&rank);
        kernels[5].setArg(2,sizeof(char),&ranks);

    }
    else
    {

        char ranks=2;
        char rt_s_0[2]={2,1}; //from CK_S_0 we go to the other CK_S if we have to ho to rank 0
        char rt_s_1[2]={0,1}; //from CK_S_1 we go to this QSFP if we have to go to rank 0
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,2,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,2,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={2,100}; //the first  is connect to application endpoint (TAG=0)
        char rt_r_1[2]={100,100};   //the second doesn't do anything
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,2,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,2,rt_r_1);

        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);

        //args for the CK_Ss
        kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[1].setArg(1,sizeof(char),&ranks);
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[2].setArg(1,sizeof(char),&ranks);

        //args for the CK_Rs
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[3].setArg(1,sizeof(char),&rank);
        kernels[3].setArg(2,sizeof(char),&ranks);
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[4].setArg(1,sizeof(char),&rank);
        kernels[4].setArg(2,sizeof(char),&ranks);


    }


    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size();i++)
        queues[i].enqueueTask(kernels[i]);


    queues[0].finish();
    if(rank==0)
        queues[1].finish();


    timestamp_t end=current_time_usecs();

    std::cout << "Sleeping a bit to ensure CK have finished their job"<<std::endl;
    sleep(1);
    if(rank==0)
    {
        queues[0].enqueueReadBuffer(check,CL_TRUE,0,1,&res);
        if(res==1)
            cout << "OK!"<<endl;

        cout << "Time elapsed (usecs): "<<end-start<<endl;
    }

}
