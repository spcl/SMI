/**
    Latency benchmark:
    a ping-pong occurs between two ranks using channels of size 1.
    The latency is computed as half of the RTT.
 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <utils/ocl_utils.hpp>
#include <utils/utils.hpp>
#include <limits.h>
#include <cmath>
#include "smi_generated_host.c"
#define ROUTING_DIR "smi-routes/"


using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: mpirun -np <num_ranks>"<< argv[0]<<" -m <emulator/hardware> -n <length> -r <rank on which run the receiver> -i <number of runs> [-b <binary file>]"<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    int recv_rank;
    int fpga,runs;
    int rank;
    bool emulator;
    bool binary_file_provided=false;

    while ((c = getopt (argc, argv, "m:n:b:r:f:i:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'i':
                runs=atoi(optarg);
                break;
            case 'm':
            {
                std::string mode=std::string(optarg);
                if(mode!="emulator" && mode!="hardware")
                {
                    cerr << "Mode: emulator or hardware"<<endl;
                    exit(-1);
                }
                emulator=mode=="emulator";
                break;
            }
            case 'b':
                program_path=std::string(optarg);
                binary_file_provided=true;
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
                cerr << "Usage: "<< argv[0]<<"-m <emulator/hardware> -b <binary file> -n <length> -r <rank on which run the receiver> -i <number of runs>"<<endl;
                exit(-1);
        }

    cout << "Performing send/receive test with "<<n<<" elements"<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // which fpga select depends on the target cluster
    if(binary_file_provided)
    {
        if(!emulator)
        {
            if(rank==0)
                program_path = replace(program_path, "<rank>", std::string("0"));
            else //any rank other than 0
                program_path = replace(program_path, "<rank>", std::string("1"));
        }
        else//for emulation
        {
            program_path = replace(program_path, "<rank>", std::to_string(rank));
            if(rank==0)
                program_path = replace(program_path, "<type>", std::string("0"));
            else
                program_path = replace(program_path, "<type>", std::string("1"));
        }
    }
    else
    {
        if(emulator)
        {
            program_path = ("emulator_" + std::to_string(rank));
            if(rank==0) program_path+="/latency_0.aocx";
            else program_path+="/latency_1.aocx";
        }
        else
        {
            if(rank==0) program_path="latency_0/latency_0.aocx";
            else program_path="latency_1/latency_1.aocx";
        }

    }

    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    std::cout << "Rank" << rank<<" executing on host:" <<hostname << " program: "<<program_path<<std::endl;


    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Buffer> buffers;
    SMI_Comm comm=SmiInit_latency_0(rank, rank_count, program_path.c_str(), ROUTING_DIR, platform, device, context, program, fpga,buffers);
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"app",kernel);

    if(rank==0)
    {
        char dest=(char)recv_rank;
        kernel.setArg(0,sizeof(int),&n);
        kernel.setArg(1,sizeof(char),&dest);
        kernel.setArg(2,sizeof(SMI_Comm),&comm);
    }
    else
    {
        kernel.setArg(0,sizeof(int),&n);
        kernel.setArg(1,sizeof(SMI_Comm),&comm);
    }
    std::vector<double> times;
    for(int i=0;i<runs;i++)
    {

        cl::Event event; 
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        //only rank 0 and the recv rank start the app kernels
        timestamp_t startt=current_time_usecs();
        //
        if(rank==0 || rank==recv_rank)
        {
            queue.enqueueTask(kernel,nullptr,&event);
            queue.finish();
        }
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==0)
        {
            ulong end, start;
            event.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            event.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
            double time= (double)((end-start)/1000.0f);
            times.push_back(time/(2*n));
        }
    }

    if(rank==0)
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
        cout << "-------------------------------------------------------------------"<<std::endl;
        cout << "Average Latency (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "-------------------------------------------------------------------"<<std::endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "latency.dat";
        ofstream fout(filename.str());
        fout << "#Average Latency (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Execution times (usecs):"<<endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();
    }
    CHECK_MPI(MPI_Finalize());

}
