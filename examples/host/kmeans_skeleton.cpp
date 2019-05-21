

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"
#define ROUTING_DIR "examples/host/k_means_routing/"
//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Reduce benchmark " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <who is the root> -i <number of iterations> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char root;
    int fpga,iterations;
    int rank;
    while ((c = getopt (argc, argv, "n:b:r:i:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'i':
                iterations=atoi(optarg);
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

    cout << "Performing reduce with  "<<1<<" elements, root: "<<(char)root<< " (attention: lenght is currently used to express the number of iteration)"<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    //fpga=0; //executed on 15 and 16
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << " executed on fpga: "<<fpga<<std::endl;
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    printf("Rank %d executing on host: %s\n",rank,hostname);
    float power;
    size_t returnedSize;


    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"CK_S_0", "CK_S_1", "CK_S_2", "CK_S_3", "CK_R_0", "CK_R_1", "CK_R_2", "CK_R_3",
                                          "kernel_reduce", "kernel_bcast", "app"};

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char tags=3;
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);

    //load ck_r
    char routing_tables_ckr[4][tags]; //two tags
    char routing_tables_cks[4][rank_count]; //4 ranks
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, tags, ROUTING_DIR, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
    }
//    std::cout << "Rank: "<< rank<<endl;
//    for(int i=0;i<kChannelsPerRank;i++)
//        for(int j=0;j<rank_count;j++)
//            std::cout << i<< "," << j<<": "<< (int)routing_tables_cks[i][j]<<endl;
//    sleep(rank);

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,&routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,&routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,&routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,&routing_tables_ckr[3][0]);


    //args for the CK_Ss
    kernels[0].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);

    //args for the CK_Rs
    kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[4].setArg(1,sizeof(char),&rank);
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[5].setArg(1,sizeof(char),&rank);
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[6].setArg(1,sizeof(char),&rank);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[7].setArg(1,sizeof(char),&rank);

    //start the CKs
    for(int i=0;i<8;i++) //start also the bcast kernel
        queues[i].enqueueTask(kernels[i]);

    //start support kernel for collectives
    kernels[8].setArg(0,sizeof(char),&rank_count);
    kernels[9].setArg(0,sizeof(char),&rank_count);

    queues[8].enqueueTask(kernels[8]);
    queues[9].enqueueTask(kernels[9]);


    //Application
    cl::Buffer reduced_elements(context,CL_MEM_READ_WRITE,n*sizeof(float));

    kernels[10].setArg(0,sizeof(int),&iterations);
    kernels[10].setArg(1,sizeof(int),&n);
    kernels[10].setArg(2,sizeof(char),&root);
    kernels[10].setArg(3,sizeof(char),&rank);
    kernels[10].setArg(4,sizeof(char),&rank_count);
    kernels[10].setArg(5,sizeof(cl_mem),&reduced_elements);
    kernels[10].setArg(6,sizeof(cl_mem),&check);



    std::vector<double> times;

  //  kernels[0].setArg(5,sizeof(int),&i);
    cl::Event events[1]; //this defination must stay here
    // wait for other nodes
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));


    queues[10].enqueueTask(kernels[10],nullptr,&events[0]);

    queues[10].finish();


    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    if(rank==root)
    {
        ulong end, start;
        events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
        events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
        double time= (double)((end-start)/1000.0f);
        times.push_back(time);
        char res;
        queues[10].enqueueReadBuffer(check,CL_TRUE,0,1,&res);
        if(res==1)
            cout << "Result is Ok!"<<endl;
        else
            cout << "Error!!!!"<<endl;


    }

    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    if(rank==root)
    {

       //check
        double mean=0;
        for(auto t:times)
            mean+=t;
        mean/=iterations;
        //report the mean in usecs
        double stddev=0;
        for(auto t:times)
            stddev+=((t-mean)*(t-mean));
        stddev=sqrt(stddev/iterations);
        double conf_interval_99=2.58*stddev/sqrt(iterations);
        double data_sent_KB=(double)(n*sizeof(float))/1024.0;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;



    }


    CHECK_MPI(MPI_Finalize());

}
