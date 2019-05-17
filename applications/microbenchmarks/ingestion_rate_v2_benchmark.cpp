/**
    Ingestion benchmark v2
    Generic CkS-CKr
    Polling schema with repeated reads


 */

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cmath>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"
#define ROUTING_DIR "applications/microbenchmarks/scaling_benchmark/"

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Ingestion rate " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <rank on which run the receiver> -i <number of runs> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    int rank;
    char recv_rank;
    int fpga,runs;
    while ((c = getopt (argc, argv, "n:b:r:i:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'i':
                runs=atoi(optarg);
                break;
            case 'r':
                recv_rank=atoi(optarg);
                break;
            default:
                cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <rank on which run the receiver> -i <number of runs> "<<endl;
                exit(-1);
        }

    cout << "Performing ingestion rate test with "<<n<<" elements"<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    /*if(rank==0)
        program_path = replace(program_path, "<rank>", std::string("0"));
    else
        program_path = replace(program_path, "<rank>", std::string("1"));
*/
    //for emulation
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << std::endl;
    char hostname[256];
    gethostname(hostname, 256);
    printf("Rank %d executing on host: %s\n",rank,hostname);

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    kernel_names.push_back("app_0");
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
    const char tags=1;
    std::cout << "Version with " <<(int)tags<< " tags" << std::endl;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer check2(context,CL_MEM_WRITE_ONLY,1);

    //load routing tables
    char routing_tables_ckr[4][tags]; //only one tag
    char routing_tables_cks[4][rank_count]; //4 ranks
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, tags, ROUTING_DIR, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
    }

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,&routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,&routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,&routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,&routing_tables_ckr[3][0]);

    kernels[0].setArg(0,sizeof(int),&n);
    if(rank==0)
        kernels[0].setArg(1,sizeof(char),&recv_rank);

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

    const int num_kernels=kernel_names.size();
    for(int i=num_kernels-1;i>=num_kernels-8;i--)
        queues[i].enqueueTask(kernels[i]);

    std::vector<double> times;

    for(int i=0;i<runs;i++)
    {


        cl::Event events[1]; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        //ATTENTION: If you are executing on the same host
        //since the PCIe is shared that could be problems in taking times
        //This mini sleep should resolve
        if(rank==0)
            usleep(10000);

        timestamp_t start=current_time_usecs();
        //TODO fix this
        /*
            Normal version
        for(int i=0;i<kernel_names.size()-8;i++)
            queues[i].enqueueTask(kernels[i],nullptr,&events[i]);

        for(int i=0;i<kernel_names.size()-8;i++)
            queues[i].finish();
        */
        //single QSFP version
        if(rank==0 || rank==recv_rank)
        {
            printf("Rank %d starting the application\n",rank);
            queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
            queues[0].finish();
        }
        timestamp_t end=current_time_usecs();

        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==0)
        {
            ulong end, start;
            events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);

            double time= (double)((end-start)/1000.0f);
            times.push_back(time);
        }
    }
    printf("Rank %d finished\n",rank);
    if(rank==0)
    {

       //check
        double mean=0;
        for(auto t:times)
            mean+=t;
        mean/=runs;
        //report the mean in usecs
        double stddev=0;
        for(auto t:times)
            stddev+=((t-mean)*(t-mean));
        stddev=sqrt(stddev/runs);
        double conf_interval_99=2.58*stddev/sqrt(runs);
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "ingestion_rate_" << n << ".dat";
        std::cout << "Saving info into: "<<filename.str()<<std::endl;
        ofstream fout(filename.str());
        fout << "#Ingestion rate benchnmark: sending  "<<n<<" messages of size 1" <<std::endl;
        fout << "#Average Computation time (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Execution times (usecs):"<<endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();
    }
    CHECK_MPI(MPI_Finalize());

}
