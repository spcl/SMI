/**
    Scaling benchmark

 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include <limits.h>
#include <cmath>
#include "codegen_scaling/smi-host-0.h"
#include "../../include/smi/communicator.h"
#define ROUTING_DIR "applications/microbenchmarks/codegen_scaling/"

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -k <KB> -r <rank on which run the receiver> -i <number of runs>"<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    int recv_rank;
    int fpga,runs;
    int rank;
    int kb;
    while ((c = getopt (argc, argv, "k:b:r:f:i:")) != -1)
        switch (c)
        {
            case 'k':
                kb=atoi(optarg);
                n=(int)ceil(kb*54.8571); //the payload of each network packet is 28B, on each packet there is space for 3 doubles
                break;
            case 'i':
                runs=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'f':
                fpga=atoi(optarg);
                break;
            case 'r':
                {
                    recv_rank=atoi(optarg);
                    break;
                }

            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }
    cout << "Performing scaling test with "<<n<<" elements per app"<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    //fpga=0; //executed on 15 and 16
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks, executing on fpga " <<fpga<< std::endl;
    /*if(rank==0)
        program_path = replace(program_path, "<rank>", std::string("0"));
    else
        program_path = replace(program_path, "<rank>", std::string("1"));
*/
    //for emulation
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << std::endl;
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    printf("Rank %d executing on host: %s\n",rank,hostname);

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Buffer> buffers;
    SMI_Comm comm=SmiInit(rank, rank_count, program_path.c_str(), ROUTING_DIR, platform, device, context, program, fpga,buffers);
    printf("Rank: %d, communitor.my_rank:%d, num_rank:%d\n",rank,(char)comm.s[0],(char)comm.s[1]);
    cl::Kernel kernels[2];
    cl::CommandQueue queues[2];
    IntelFPGAOCLUtils::createCommandQueue(context,device,queues[0]);
    IntelFPGAOCLUtils::createCommandQueue(context,device,queues[1]);
    IntelFPGAOCLUtils::createKernel(program,"app",kernels[0]);
    IntelFPGAOCLUtils::createKernel(program,"app_1",kernels[1]);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer check2(context,CL_MEM_WRITE_ONLY,1);

    if(rank==0)
    {
        char dest=(char)recv_rank;
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[0].setArg(1,sizeof(char),&dest);
        kernels[0].setArg(2,sizeof(SMI_Comm),&comm);
        kernels[1].setArg(0,sizeof(int),&n);
        kernels[1].setArg(1,sizeof(char),&dest);
        kernels[1].setArg(2,sizeof(SMI_Comm),&comm);

    }
    else
    {
        kernels[0].setArg(0,sizeof(cl_mem),&check);
        kernels[0].setArg(1,sizeof(int),&n);
        kernels[0].setArg(2,sizeof(SMI_Comm),&comm);
        kernels[1].setArg(0,sizeof(cl_mem),&check2);
        kernels[1].setArg(1,sizeof(int),&n);
        kernels[1].setArg(2,sizeof(SMI_Comm),&comm);
    }
    std::vector<double> times;
    for(int i=0;i<runs;i++)
    {

        cl::Event events[2]; //this defination must stay here
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        //ATTENTION: If you are executing on the same host
        //since the PCIe is shared that could be problems in taking times
        //This mini sleep should resolve

        //only rank 0 and the recv rank start the app kernels
        timestamp_t startt=current_time_usecs();
        //
        if(rank==0 || rank==recv_rank)
        {
            queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
            queues[1].enqueueTask(kernels[1],nullptr,&events[1]);

            queues[0].finish();
            queues[1].finish();
        }
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==recv_rank)
        {
            ulong min_start=4294967295, max_end=0;
            ulong end, start;
            for(int i=0;i<2;i++)
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
            times.push_back((double)((max_end-min_start)/1000.0f));

            //check
            char res,res2;
            queues[0].enqueueReadBuffer(check,CL_TRUE,0,1,&res);
            queues[0].enqueueReadBuffer(check2,CL_TRUE,0,1,&res2);
            if(res==1 && res2==1)
                cout << "Result is Ok!"<<endl;
            else
                cout << "Error!!!!"<<endl;

        }
    }

    if(rank==recv_rank)
    {
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
        double data_sent_KB=ceil(n/3)*2*28/1024; //the real amount of data sent (payload)
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "smi_scaling_"<<rank_count-1 <<"_hops_"<< kb << "KB.dat";
        std::cout << "Saving info into: "<<filename.str()<<std::endl;
        ofstream fout(filename.str());
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
