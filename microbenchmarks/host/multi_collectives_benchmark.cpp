/**
 * Evaluation of the benefits of simultaneous collectives
 * In this benchmark we evaluate the composition of a reduce and a broadcast (all reduce).
 * In the first case, the two collectives are executed one after the other.
 * In the second they are executed simultaenously
*/

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include <utils/ocl_utils.hpp>
#include "smi_generated_host.c"
#define ROUTING_DIR "smi-routes/"

using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Multi-collectives benchmark " <<endl;
        cerr << "Usage: "<< argv[0]<<" -m <emulator/hardware> -n <length> -r <who is the root> -i <number of iterations> [-b \"<binary file>\"]"<<endl;
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
    while ((c = getopt (argc, argv, "n:b:r:i:m:")) != -1)
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
            case 'r':
                root=(char)atoi(optarg);
                break;

            default:
                cerr << "Usage: "<< argv[0]<<" -n <length> -r <who is the root> -i <number of iterations> [-b <binary file>]"<<endl;
                exit(-1);
        }

    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2;
    if(binary_file_provided)
        program_path = replace(program_path, "<rank>", std::to_string(rank));
    else
    {
        if(emulator)
            program_path = ("emulator_" + std::to_string(rank) +"/multi_collectives.aocx");
        else
            program_path = "multi_collectives.aocx";
    }
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    std::cout << "Rank" << rank<<" executing on host:" <<hostname << " program: "<<program_path<<std::endl;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Buffer> buffers;
    SMI_Comm comm=SmiInit_multi_collectives(rank, rank_count, program_path.c_str(), ROUTING_DIR, platform, device, context, program, fpga,buffers);

    /*----------------------------------------------
     * Sequential collectives
     * ----------------------------------------------*/
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"sequential_collectives",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);

    kernel.setArg(0,sizeof(int),&n);
    kernel.setArg(1,sizeof(char),&root);
    kernel.setArg(2,sizeof(cl_mem),&check);
    kernel.setArg(3,sizeof(SMI_Comm),&comm);
    std::vector<double> times_sequential;

    for(int i=0;i<runs;i++)
    {
        if(rank==0)  //remove emulated channels
            system("rm emulated_chan* 2> /dev/null;");
        cl::Event events;
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
            times_sequential.push_back(time);
            char res;
            queue.enqueueReadBuffer(check,CL_TRUE,0,1,&res);
            if(res==1)
                cout << "Result is Ok!"<<endl;
            else
                cout << "Error!!!!"<<endl;
        }
    }
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    /*------------------------------------------------------
     * Simultaneous Collectives
     * ----------------------------------------------------*/
    cl::Kernel kernel_sim;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"simultaneous_collectives",kernel_sim);

    std::vector<double> times_simultaneous;

    kernel_sim.setArg(0,sizeof(int),&n);
    kernel_sim.setArg(1,sizeof(char),&root);
    kernel_sim.setArg(2,sizeof(cl_mem),&check);
    kernel_sim.setArg(3,sizeof(SMI_Comm),&comm);

    for(int i=0;i<runs;i++)
    {
        if(rank==0)  //remove emulated channels
            system("rm emulated_chan* 2> /dev/null;");
        cl::Event events;
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        queue.enqueueTask(kernel_sim,nullptr,&events);
        queue.finish();

        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==root)
        {
            ulong end, start;
            events.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            events.getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
            double time= (double)((end-start)/1000.0f);
            times_simultaneous.push_back(time);
            char res;
            queue.enqueueReadBuffer(check,CL_TRUE,0,1,&res);
            if(res==1)
                cout << "Result is Ok!"<<endl;
            else
                cout << "Error!!!!"<<endl;
        }
    }


    if(rank==root)
    {
        double mean_seq=0;
        for(auto t:times_sequential)
            mean_seq+=t;
        mean_seq/=runs;
        //report the mean in usecs
        double stddev_seq=0;
        for(auto t:times_sequential)
            stddev_seq+=((t-mean_seq)*(t-mean_seq));
        stddev_seq=sqrt(stddev_seq/runs);
        double conf_interval_99_seq=2.58*stddev_seq/sqrt(runs);
        double data_sent_KB=(double)(n*sizeof(float))/1024.0;
        cout << "------------------- Sequential collectives ------------------------"<<std::endl;
        cout << "Computation time (usec): " << mean_seq << " (sttdev: " << stddev_seq<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99_seq<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99_seq/mean_seq)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean_seq/1000000.0))/(1024*1024) << endl;
        cout << "-------------------------------------------------------------------"<<std::endl;
        cout << endl;

        double mean_sim=0;
        for(auto t:times_simultaneous)
            mean_sim+=t;
        mean_sim/=runs;
        //report the mean in usecs
        double stddev_sim=0;
        for(auto t:times_simultaneous)
            stddev_sim+=((t-mean_sim)*(t-mean_sim));
        stddev_sim=sqrt(stddev_sim/runs);
        double conf_interval_99_sim=2.58*stddev_sim/sqrt(runs);
        cout << "------------------- Simultanoues collectives ------------------------"<<std::endl;
        cout << "Computation time (usec): " << mean_sim << " (sttdev: " << stddev_sim<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99_sim<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99_sim/mean_sim)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean_sim/1000000.0))/(1024*1024) << endl;
        cout << "-------------------------------------------------------------------"<<std::endl;

        //save the info into output file
        /*std::ostringstream filename;
        filename << "smi_multi_collectvies_"<<rank_count <<"_"<< n << ".dat";
        std::cout << "Saving info into: "<<filename.str()<<std::endl;
        ofstream fout(filename.str());
        fout << "#SMI Reduce, executed with " << rank_count <<" ranks, streaming: " << n <<" elements"<<endl;
        fout << "#Sent (KB) = "<<data_sent_KB<<", Runs = "<<runs<<endl;
        fout << "#Average Computation time (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Execution times (usecs):"<<endl;
        fout << "#Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();*/
    }
    CHECK_MPI(MPI_Finalize());
}
