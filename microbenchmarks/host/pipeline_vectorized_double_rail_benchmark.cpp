/**
    Pipeline benchmark: each rank receives from the previous one (if any) and sends to the next one (if any)

 */

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include <utils/ocl_utils.hpp>
#include <utils/utils.hpp>
#include "smi_generated_host.c"
#include <hlslib/intel/OpenCL.h>
#define ROUTING_DIR "smi-routes/"


using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<7)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -m <emulator/hardware> -n <length>  -i <number of iterations> [-b \"<binary file>\"]"<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char root;
    int fpga,runs;
    int rank;
    bool binary_file_provided=false;
    bool emulator;
    while ((c = getopt (argc, argv, "n:b:i:m:")) != -1)
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

            default:
                cerr << "Usage: "<< argv[0]<<"-m <emulator/hardware> -n <length> -i <number of iterations> [-b <binary file>]"<<endl;
                exit(-1);
        }

    cout << "Performing pipeline double rail with  "<<n<<" elements."<<endl;
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    fpga = rank % 2;
    if(binary_file_provided)
        program_path = replace(program_path, "<rank>", std::to_string(rank));
    else
    {
        if(emulator)
            program_path = ("emulator_" + std::to_string(rank) +"/pipeline_vectorized_double_rail.aocx");
        else
            program_path = "pipeline/pipeline_vectorized_double_rail.aocx";
    }       char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    std::cout << "Rank" << rank<<" executing on host:" <<hostname << " program: "<<program_path<<std::endl;

    hlslib::ocl::Context context(fpga);
    auto program = context.MakeProgram(program_path);
    std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>> buffers;
    SMI_Comm comm=SmiInit_pipeline_vectorized_double_rail(rank, rank_count, ROUTING_DIR, context, program, buffers);

    // Create device buffer
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context.MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);

    // Create kernel
    hlslib::ocl::Kernel kernel = program.MakeKernel("app", check, n, comm);

    std::vector<double> times;
    for(int i=0;i<runs;i++)
    {
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        std::pair<double, double> timings = kernel.ExecuteTask();

        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==rank_count-1)
            times.push_back(timings.first * 1e6);

        if(rank==rank_count-1)
        {
            char res;
            check.CopyToHost(&res);
            if(res==1)
                cout << "Rank: " << rank<<" Result is Ok!"<<endl;
            else
                cout << "Rank: " << rank<<" Error!!!!"<<endl;
        }
    }
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    if(rank==rank_count-1)
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
        cout << "-------------------------------------------------------------------"<<std::endl;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;
        cout << "-------------------------------------------------------------------"<<std::endl;

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
