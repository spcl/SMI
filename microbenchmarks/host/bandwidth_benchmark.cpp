/**
    Bandwidth benchmark
    Two ranks exchange data and bandwidth is measured
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
#include <hlslib/intel/OpenCL.h>
#define ROUTING_DIR "smi-routes/"
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Bandwidth benchmark " <<endl;
        cerr << "Usage: mpirun -np <num_rank>"<< argv[0]<<" -m <emulator/hardware> -k <KB> -r <rank on which run the receiver> -i <number of runs> [-b \"<binary file>\"]"<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    int recv_rank;
    int fpga,runs;
    int rank;
    int kb;
    bool emulator;
    bool binary_file_provided=false;
    while ((c = getopt (argc, argv, "k:b:r:f:i:m:")) != -1)
        switch (c)
        {
            case 'k':
                kb=atoi(optarg);
                n=(int)ceil(kb*54.8571); //the payload of each network packet is 28B
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
            case 'i':
                runs=atoi(optarg);
                break;
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
                cerr << "Usage: "<< argv[0]<<" -m <emulator/hardware> -b <binary file> -k <KB> -r <rank on which run the receiver> -i <number of runs>"<<endl;
                exit(-1);
        }
    if(rank == 0)
    cout << "Performing bandwidth test with "<<n<<" elements per app"<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2;
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
            if(rank==0) program_path+="/bandwidth_0.aocx";
            else program_path+="/bandwidth_1.aocx";
        }
        else
        {
            if(rank==0) program_path="bandwidth_0/bandwidth_0.aocx";
            else program_path="bandwidth_1/bandwidth_1.aocx";
        }

    }
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    std::cout << "Rank" << rank<<" executing on host:" <<hostname << " program: "<<program_path<<std::endl;

    hlslib::ocl::Context context(fpga);
    auto program = context.MakeProgram(program_path);
    std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>> buffers;
    SMI_Comm comm=SmiInit_bandwidth_0(rank, rank_count, ROUTING_DIR, context, program, buffers);

     // Create device buffers
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check_0 = context.MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check_1 = context.MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);

     // Create kernel
    char dest=(char)recv_rank;
    hlslib::ocl::Kernel app_0 = (rank==0)?program.MakeKernel("app", n, dest,  comm) : program.MakeKernel("app", check_0, n,comm) ;
    hlslib::ocl::Kernel app_1 = (rank==0)?program.MakeKernel("app_1", n, dest,  comm) :program.MakeKernel("app_1", check_1, n, comm)  ;

    std::vector<double> times;
    for(int i=0;i<runs;i++)
    {

        cl::Event events[2];
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        //only rank 0 and the recv rank start the app kernels
        std::future<std::pair<double, double>>  fut_0, fut_1;

        if(rank==0 || rank==recv_rank)
        {
            fut_0 = app_0.ExecuteTaskAsync();
            fut_1 = app_1.ExecuteTaskAsync();
            fut_0.wait();
            fut_1.wait();
        }
        std::pair<double, double> timings_0 = fut_0.get();
        std::pair<double, double> timings_1 = fut_1.get();
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==recv_rank)
        {
            if (timings_0.first>timings_1.first)
                times.push_back(timings_0.first * 1e6);
            else
                times.push_back(timings_1.first * 1e6);

            //check
            char res_0,res_1;
            check_0.CopyToHost(&res_0);
            check_1.CopyToHost(&res_1);
            if(res_0==1 && res_1==1)
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
        double data_sent_KB=ceil(n/3)*2*28/1024; //the amount of data sent (payload)
        cout << "-------------------------------------------------------------------"<<std::endl;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;
        cout << "-------------------------------------------------------------------"<<std::endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "smi_bandwidth_"<<rank_count-1 <<"_hops_"<< kb << "KB.dat";
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
