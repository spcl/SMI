/**
    Broadcast benchmark

 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"
#define ROUTING_DIR "applications/microbenchmarks/broadcast_routing/"
//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<7)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <who is the root> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char root;
    int fpga;
    int rank;
    while ((c = getopt (argc, argv, "n:b:r:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'r':
                root=(char)atoi(optarg);
                break;

            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cout << "Performing broadcast wit  "<<n<<" elements, root: "<<(char)root<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    //fpga=0; //executed on 15 and 16
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << " executed on fpga: "<<fpga<<std::endl;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;
    kernel_names.push_back("app");
    kernel_names.push_back("CK_S_0");
    kernel_names.push_back("CK_S_1");   
    kernel_names.push_back("CK_S_2");
    kernel_names.push_back("CK_S_3");
    kernel_names.push_back("CK_R_0");
    kernel_names.push_back("CK_R_1");
    kernel_names.push_back("CK_R_2");
    kernel_names.push_back("CK_R_3");


    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char tags=1;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);

    //load ck_r
    char routing_tables_ckr[4]; //only one tag
    char routing_tables_cks[4][rank_count]; //4 ranks 
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, 1, ROUTING_DIR, "ckr", &routing_tables_ckr[i]);
        LoadRoutingTable<char>(rank, i, rank_count, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
    }
    //std::cout << "Rank: "<< rank<<endl;
   // for(int i=0;i<kChannelsPerRank;i++)
    //    for(int j=0;j<rank_count;j++)
    //        std::cout << i<< "," << j<<": "<< (int)routing_tables_cks[i][j]<<endl;

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,&routing_tables_ckr[0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,&routing_tables_ckr[1]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,&routing_tables_ckr[2]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,&routing_tables_ckr[3]);
    
    kernels[0].setArg(0,sizeof(int),&n);
    kernels[0].setArg(1,sizeof(char),&root);
    kernels[0].setArg(2,sizeof(char),&rank);
    kernels[0].setArg(3,sizeof(char),&rank_count);

    //args for the CK_Ss
    kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);

    //args for the CK_Rs
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[5].setArg(1,sizeof(char),&rank);
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[6].setArg(1,sizeof(char),&rank);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[7].setArg(1,sizeof(char),&rank);
    kernels[8].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[8].setArg(1,sizeof(char),&rank);

    //start the CKs
    const int num_kernels=kernel_names.size();
    for(int i=num_kernels-1;i>=num_kernels-8;i--)
        queues[i].enqueueTask(kernels[i]);

    cl::Event events[1]; //this defination must stay here
    // wait for other nodes
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    //ATTENTION: If you are executing on the same host
    //since the PCIe is shared that could be problems in taking times
    //This mini sleep should resolve
    //if(rank==0)
    //    usleep(10000);

    timestamp_t start=current_time_usecs();
    for(int i=0;i<1;i++)
        queues[i].enqueueTask(kernels[i],nullptr,&events[i]);
    
    for(int i=0;i<1;i++)
        queues[i].finish();

    timestamp_t end=current_time_usecs();

    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    if(rank==0)
    {
        ulong min_start=4294967295, max_end=0;
        ulong end, start;
        for(int i=0;i<1;i++)
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
    CHECK_MPI(MPI_Finalize());

}
