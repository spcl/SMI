/**
    Reduce benchmark

 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include "../../include/utils/ocl_utils.hpp"
#include "reduce_codegen/smi-host-0.h"
#define ROUTING_DIR "applications/microbenchmarks/reduce_codegen/"

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

    SmiInit(rank, rank_count, program_path.c_str(), ROUTING_DIR, platform, device, context, program, fpga);

    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"app",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);


   
    kernel.setArg(0,sizeof(int),&n);
    kernel.setArg(1,sizeof(char),&root);
    kernel.setArg(2,sizeof(char),&rank);
    kernel.setArg(3,sizeof(char),&rank_count);
    kernel.setArg(4,sizeof(cl_mem),&check);
    std::vector<double> times;
    for(int i=0;i<runs;i++)
    {
        cl::Event events; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        queue.enqueueTask(kernel,nullptr,&events);

        queue.finish();


        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==root)
        {
            ulong end, start;
            events.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            events.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
            double time= (double)((end-start)/1000.0f);
            times.push_back(time);
            char res;
            queue.enqueueReadBuffer(check,CL_TRUE,0,1,&res);
            if(res==1)
                cout << "Result is Ok!"<<endl;
            else
                cout << "Error!!!!"<<endl;


        }
    }
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    if(rank==root)
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
        filename << "smi_reduce_"<<rank_count <<"_"<< n << ".dat";
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
