/**
    Broadcast benchmark, host based:
    - data is prepared by rank 0 (written in memory, using host_broadcast_rank0.cl)
    - then it is copied into host and broadcasted to all the others
    - whose in turns copy it to device and execute host_broadcast_rank1.cl

 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cmath>
#include <sstream>
#include <limits.h>

#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<7)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file with <type> tag> -n <lenght> -r <num runs> "<<endl;
        exit(-1);
    }
    int KB;
    int c;
    int n;
    std::string program_path;
    int runs;
    int fpga;
    while ((c = getopt (argc, argv, "n:b:r:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'r':
                runs=atoi(optarg);
                break;
            default:
                cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length>"<<endl;
                exit(-1);
        }
    KB=n*4/1024;
    cout << "Performing send/receive test with "<<n<<" elements"<<endl;
    int rank_count;
    int rank;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    //fpga=0; //executed on 15 and 16
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    if(rank_count<4)
    {
        std::cout << "You have to execut this with 4 ranks" <<std::endl;
        exit(-1);
    }
    if(rank==0)
        program_path = replace(program_path, "<type>", std::string("root"));
    else
        program_path = replace(program_path, "<type>", std::string("rank"));
    std::cout << "Program: " << program_path << std::endl;
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    printf("Rank %d executing on host: %s\n",rank,hostname);


    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"app_0"};

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    double *host_data;
    //ATTENTION: declare all the memory here otherwise it hangs
    cl::Buffer mem(context,CL_MEM_READ_WRITE,max(n,1)*sizeof(float));
    posix_memalign ((void **)&host_data, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));

    kernels[0].setArg(0,sizeof(cl_mem),&mem);
    kernels[0].setArg(1,sizeof(int),&n);

    std::vector<double> times, transfers;
    timestamp_t startt,endt;
    //Program startup
    for(int i=0;i<runs;i++)
    {
        cl::Event events[3]; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        startt=current_time_usecs();
        if(rank==0)
        {
            queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
            queues[0].finish();
            //get the result and send it  to rank 1
            timestamp_t transf_start=current_time_usecs();
            queues[0].enqueueReadBuffer(mem,CL_TRUE,0,n*sizeof(float),host_data);
            transfers.push_back(current_time_usecs()-transf_start);
            MPI_Bcast(host_data, n, MPI_FLOAT, 0, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Bcast(host_data, n, MPI_FLOAT, 0, MPI_COMM_WORLD);
            //copy and start axpy
            timestamp_t transf_start=current_time_usecs();
            queues[0].enqueueWriteBuffer(mem,CL_TRUE,0,n*sizeof(float),host_data);
            transfers.push_back(current_time_usecs()-transf_start);
            queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
            queues[0].finish();
            endt=current_time_usecs();
            times.push_back(endt-startt);
        }

        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    }
    if(rank==1)//compute the time not on the root
    {

       //check
        double mean=0;
        for(auto t:times)
            mean+=t;
        mean/=runs;
        //compute the average transfer time for this rank. Experimentally, the times are the same for both ranks
        double transfer_time=0;
        for(auto t:transfers)
            transfer_time+=t;
        transfer_time/=runs;
        //report the mean in usecs
        double stddev=0;
        for(auto t:times)
            stddev+=((t-mean)*(t-mean));
        stddev=sqrt(stddev/runs);
        double conf_interval_99=2.58*stddev/sqrt(runs);
        double data_sent_KB=KB;
        double comp_time_no_pcie=mean-2*transfer_time;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Average transfer time PCI-E (usecs): "<<transfer_time*2<<endl;
        cout << "Computation time without PCI-E (usec): " <<comp_time_no_pcie<< " (sttdev: " << stddev<<")"<<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;
        cout << "Average bandwidth without PCI-e(Gbit/s): " <<  (data_sent_KB*8/((comp_time_no_pcie)/1000000.0))/(1024*1024) << endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "host_based_broadcast_" << n << ".dat";
	std::cout << "Saving info into: "<<filename.str()<<std::endl;
        ofstream fout(filename.str());
        fout << "#Sent (KB) = "<<data_sent_KB<<", Runs = "<<runs<<endl;
        fout << "#Average Computation time (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Average transfer time PCI-E (usecs): "<<transfer_time*2<<endl;
        fout << "#Computation time without PCI-E (usec): " << comp_time_no_pcie<< " (sttdev: " << stddev<<")"<<endl;
        fout << "#Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;
        fout << "#Average bandwidth without PCI-e(Gbit/s): " <<  (data_sent_KB*8/((comp_time_no_pcie)/1000000.0))/(1024*1024) << endl;

        fout << "#Execution times (usecs):"<<endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();
    }
    CHECK_MPI(MPI_Finalize());

}
