/**
    Ingestion benchmark

 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#define MPI
#if defined(MPI)
#include "../../include/utils/smi_utils.hpp"
#endif

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    #if defined(MPI)
    CHECK_MPI(MPI_Init(&argc, &argv));
    #endif

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
    int rank;
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
                        cerr << "Error: rank may be 0-1"<<endl;
                        exit(-1);
                    }

                    break;
                }

            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cout << "Performing send/receive test with "<<n<<" elements"<<endl;
    #if defined(MPI)
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << std::endl;
    #endif

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    switch(rank)
    {
        case 0:
            cout << "Starting rank 0 on FPGA "<<fpga<<endl;
            kernel_names.push_back("app_0");
            //we don't need to start all CK_S/CK_R if we don't use them
            kernel_names.push_back("CK_S_0");
            kernel_names.push_back("CK_S_1");
            kernel_names.push_back("CK_R_0");
            kernel_names.push_back("CK_R_1");
            break;
        case 1:
                cout << "Starting rank 1 on FPGA "<<fpga<<endl;
                kernel_names.push_back("app_1");
                kernel_names.push_back("app_2");
                kernel_names.push_back("CK_S_0");
                kernel_names.push_back("CK_S_1");
                kernel_names.push_back("CK_R_0");
                kernel_names.push_back("CK_R_1");

        break;

    }

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char ranks=2;
    char tags=2;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    if(rank==0)
    {
        //senders routing tables
        char rt_s_0[2]={1,0};
        char rt_s_1[2]={1,2};
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        //receivers routing tables: these are meaningless
        char rt_r_0[2]={100,100}; //the first doesn't do anything
        char rt_r_1[2]={100,100};   //the second is attached to the ck_r (channel has TAG 1)
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,rt_r_1);
        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);
        //args for the CK_Ss
        kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);

        //args for the CK_Rs
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[3].setArg(1,sizeof(char),&rank);
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[4].setArg(1,sizeof(char),&rank);

    }
    if(rank==1)
    {
        //sender routing tables are meaningless
        char rt_s_0[2]={0,0};
        char rt_s_1[2]={100,100};
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={4,1}; //the first  is connect to application endpoint (TAG=0)
        char rt_r_1[2]={100,4};
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,rt_r_1);

        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(int),&n);

        //args for the CK_Ss
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);

        //args for the CK_Rs
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[4].setArg(1,sizeof(char),&rank);
        kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[5].setArg(1,sizeof(char),&rank);

    }

    const int num_kernels=kernel_names.size();
    queues[num_kernels-1].enqueueTask(kernels[num_kernels-1]);
    queues[num_kernels-2].enqueueTask(kernels[num_kernels-2]);
    queues[num_kernels-3].enqueueTask(kernels[num_kernels-3]);
    queues[num_kernels-4].enqueueTask(kernels[num_kernels-4]);


    cl::Event events[1]; //this defination must stay here
    // wait for other nodes
    #if defined(MPI)
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    #endif

    //ATTENTION: If you are executing on the same host
    //since the PCIe is shared that could be problems in taking times
    //This mini sleep should resolve
    if(rank==0)
        usleep(10000);

    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size()-4;i++)
        queues[i].enqueueTask(kernels[i],nullptr,&events[i]);

    for(int i=0;i<kernel_names.size()-4;i++)
        queues[i].finish();

    timestamp_t end=current_time_usecs();

    #if defined(MPI)
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    #else
    sleep(1);
    #endif
    if(rank==0)
    {
        ulong min_start=4294967295, max_end=0;
        ulong end, start;
        for(int i=0;i<num_kernels-3;i++)
        {
            events[i].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            events[i].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
            if(i==0)
                min_start=start;
            if(start<min_start)
                min_start=start;
            if(end>max_end)
                max_end=end;
        }
        double time= (double)((max_end-min_start)/1000.0f);
        cout << "Time elapsed (usecs): "<<time <<endl;
        cout << "Latency (usecs): " << time/(2*n)<<endl;
    }

    #if defined(MPI)
    CHECK_MPI(MPI_Finalize());
    #endif

}
