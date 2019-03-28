/**
 * Host program for example 4.
 */
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <mpi.h>

#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"

void checkMpiCall(int code, const char* location, int line)
{
    if (code != MPI_SUCCESS)
    {
        char error[256];
        int length;
        MPI_Error_string(code, error, &length);
        std::cerr << "MPI error at " << location << ":" << line << ": " << error << std::endl;
    }
}

#define CHECK_MPI(err) checkMpiCall((err), __FILE__, __LINE__);

template <typename DataSize>
void load_routing_table(int rank, int channel, int ranks, const std::string& routing_directory,
        const std::string& prefix, DataSize* table)
{
    std::stringstream path;
    path << routing_directory << "/" << prefix << "-rank" << rank << "-channel" << channel;

    std::ifstream file(path.str(), std::ios::binary);
    if (!file)
    {
        std::cerr << "Routing table " << path.str() << " not found" << std::endl;
        std::exit(1);
    }

    auto byte_size = ranks * sizeof(DataSize);
    file.read(table, byte_size);
}

std::string replace(std::string source, const std::string& pattern, const std::string& replacement)
{
    auto pos = source.find(pattern);
    if (pos != std::string::npos)
    {
        return source.substr(0, pos) + replacement + source.substr(pos + pattern.length());
    }
    return source;
}


//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{
    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc < 7)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <routing-directory>"<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    std::string routing_directory;
    while ((c = getopt (argc, argv, "n:b:r:")) != -1)
    {
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'r':
                routing_directory=std::string(optarg);
                break;
            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }
    }

    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));

    int rank;
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    int fpga = rank % 2; // TODO: change?

    cout << "Performing send/receive test with " << n << " integers"<<endl;
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;

    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cerr << "Program: " << program_path << std::endl;

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

        load_routing_table<char>(rank, 0, rank_count, routing_directory, "ckr", rt_s_0);
        load_routing_table<char>(rank, 1, rank_count, routing_directory, "ckr", rt_s_1);

        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,2,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,2,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={100,100}; //the first doesn't do anything
        char rt_r_1[2]={0,2};   //the second is attached to the ck_r (channel has TAG 1)

        load_routing_table<char>(rank, 0, rank_count, routing_directory, "cks", rt_r_0);
        load_routing_table<char>(rank, 1, rank_count, routing_directory, "cks", rt_r_1);

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

        load_routing_table<char>(rank, 0, rank_count, routing_directory, "ckr", rt_s_0);
        load_routing_table<char>(rank, 1, rank_count, routing_directory, "ckr", rt_s_1);

        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,2,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,2,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={2,100}; //the first  is connect to application endpoint (TAG=0)
        char rt_r_1[2]={100,100};   //the second doesn't do anything

        load_routing_table<char>(rank, 0, rank_count, routing_directory, "cks", rt_r_0);
        load_routing_table<char>(rank, 1, rank_count, routing_directory, "cks", rt_r_1);

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

    // wait for other nodes
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

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

    CHECK_MPI(MPI_Finalize());
}
