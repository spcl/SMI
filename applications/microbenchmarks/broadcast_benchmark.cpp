/**
    Broadcast benchmark

 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "broadcast/smi-host-0.h"
#define ROUTING_DIR "applications/microbenchmarks/broadcast/"
//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <who is the root> -i <number of iterations> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char root;
    int fpga,runs;
    int rank;
    while ((c = getopt (argc, argv, "n:b:r:i:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'i':
                runs=atoi(optarg);
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
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    printf("Rank %d executing on host: %s\n",rank,hostname);

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Buffer> buffers;
    SmiInit(rank,rank_count,program_path.c_str(),ROUTING_DIR,platform,device,context,program,fpga,buffers);

#if 0
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    // channel kernels
    kernel_names.push_back("smi_kernel_cks_0");
    kernel_names.push_back("smi_kernel_ckr_0");
    kernel_names.push_back("smi_kernel_cks_1");
    kernel_names.push_back("smi_kernel_ckr_1");
    kernel_names.push_back("smi_kernel_cks_2");
    kernel_names.push_back("smi_kernel_ckr_2");
    kernel_names.push_back("smi_kernel_cks_3");
    kernel_names.push_back("smi_kernel_ckr_3");

    // broadcast kernels
    kernel_names.push_back("smi_kernel_bcast_0");

    // reduce kernels

    // scatter kernels

    // gather kernels


    IntelFPGAOCLUtils::initEnvironment(
            platform, device, fpga, context,
            program, program_path, kernel_names, kernels, queues
    );

    // create buffers for CKS/CKR
    const int ports = 1;
    const int cks_table_size = rank_count;
    const int ckr_table_size = ports * 2;
    cl::Buffer routing_table_ck_s_0(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_0(context, CL_MEM_READ_ONLY, ckr_table_size);
    cl::Buffer routing_table_ck_s_1(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_1(context, CL_MEM_READ_ONLY, ckr_table_size);
    cl::Buffer routing_table_ck_s_2(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_2(context, CL_MEM_READ_ONLY, ckr_table_size);
    cl::Buffer routing_table_ck_s_3(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_3(context, CL_MEM_READ_ONLY, ckr_table_size);

    // load routing tables
    std::cout << "Using " << ports << " ports" << std::endl;
    char routing_tables_cks[4][cks_table_size];
    char routing_tables_ckr[4][ckr_table_size];
    for (int i = 0; i < 4; i++)
    {
        LoadRoutingTable<char>(rank, i, cks_table_size, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
        LoadRoutingTable<char>(rank, i, ckr_table_size, ROUTING_DIR, "ckr", &routing_tables_ckr[i][0]);
    }

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE, 0, cks_table_size, &routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE, 0, cks_table_size, &routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE, 0, cks_table_size, &routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE, 0, cks_table_size, &routing_tables_cks[3][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[3][0]);

    char char_ranks_count=(char)rank_count;
    char char_rank=(char)rank;
    // cks_0
    kernels[0].setArg(0, sizeof(cl_mem), &routing_table_ck_s_0);
    kernels[0].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_0
    kernels[1].setArg(0, sizeof(cl_mem), &routing_table_ck_r_0);
    kernels[1].setArg(1, sizeof(char), &char_rank);
    // cks_1
    kernels[2].setArg(0, sizeof(cl_mem), &routing_table_ck_s_1);
    kernels[2].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_1
    kernels[3].setArg(0, sizeof(cl_mem), &routing_table_ck_r_1);
    kernels[3].setArg(1, sizeof(char), &char_rank);
    // cks_2
    kernels[4].setArg(0, sizeof(cl_mem), &routing_table_ck_s_2);
    kernels[4].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_2
    kernels[5].setArg(0, sizeof(cl_mem), &routing_table_ck_r_2);
    kernels[5].setArg(1, sizeof(char), &char_rank);
    // cks_3
    kernels[6].setArg(0, sizeof(cl_mem), &routing_table_ck_s_3);
    kernels[6].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_3
    kernels[7].setArg(0, sizeof(cl_mem), &routing_table_ck_r_3);
    kernels[7].setArg(1, sizeof(char), &char_rank);

    // broadcast 0
    kernels[8].setArg(0, sizeof(char), &char_ranks_count);




    // start the kernels
    const int num_kernels = kernel_names.size();
    for(int i = num_kernels - 1; i >= 0; i--)
    {
        queues[i].enqueueTask(kernels[i]);
        queues[i].flush();
    }
#endif
    //create the app
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"app",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);

    kernel.setArg(0,sizeof(cl_mem),&check);
    kernel.setArg(1,sizeof(int),&n);
    kernel.setArg(2,sizeof(char),&root);
    kernel.setArg(3,sizeof(char),&rank);
    kernel.setArg(4,sizeof(char),&rank_count);


    std::vector<double> times;
    for(int i=0;i<runs;i++)
    {
        cl::Event event; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        queue.enqueueTask(kernel,nullptr,&event);
        queue.finish();


        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==0)
        {
            ulong end, start;
            event.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            event.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
            double time= (double)((end-start)/1000.0f);
            times.push_back(time);
        }
        if(rank==1)
        {
            char res;
            queue.enqueueReadBuffer(check,CL_TRUE,0,1,&res);
            if(res==1)
                cout << "Result is Ok!"<<endl;
            else
                cout << "Error!!!!"<<endl;

        }
    }
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
        double data_sent_KB=(double)(n*sizeof(float))/1024.0;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "smi_broadcast_"<<rank_count <<"_"<< n << ".dat";
        std::cout << "Saving info into: "<<filename.str()<<std::endl;
        ofstream fout(filename.str());
        fout << "#SMI Broadcast, executed with " << rank_count <<" ranks, streaming: " << n <<" elements"<<endl;
        fout << "#Sent (KB) = "<<data_sent_KB<<", Runs = "<<runs<<endl;
        fout << "#Average Computation time (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Execution times (usecs):"<<endl;
        fout << "#Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();
    }


    CHECK_MPI(MPI_Finalize());

}
