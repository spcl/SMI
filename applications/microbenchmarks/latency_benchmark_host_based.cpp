/**
    Latency benchmark host based version:
    - on rank 0 the FPGA application is executed, updates a value in memory ( to simulate that the message is arrived)
    - the host copies back the result and sends it to rank 1
    - which does the same

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
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <num runs> "<<endl;
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
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cout << "Performing send/receive test with "<<n<<" elements"<<endl;
    int rank_count;
    int rank;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
   // fpga = rank % 2; // in this case is ok, pay attention
    fpga=0; //executed on 15 and 16
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    //program_path = replace(program_path, "<rank>", std::to_string(rank));
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
    std::vector<std::string> kernel_names={"app"};

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    double *host_data;
    //ATTENTION: declare all the memory here otherwise it hangs
    cl::Buffer mem(context,CL_MEM_READ_WRITE,1*sizeof(int));
    posix_memalign ((void **)&host_data, IntelFPGAOCLUtils::AOCL_ALIGNMENT, 1*sizeof(int));

    kernels[0].setArg(0,sizeof(cl_mem),&mem);

    std::vector<double> times;
    timestamp_t startt,endt;
    //Program startup
    for(int i=0;i<runs;i++)
    {
        cl::Event events[3]; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        startt=current_time_usecs();
        for(int j=0;j<n;j++)
        {
            if(rank==0)
            {
                queues[0].enqueueTask(kernels[0]);
                queues[0].finish();
                //get the result and send it  to rank 1
                queues[0].enqueueReadBuffer(mem,CL_TRUE,0,1*sizeof(int),host_data);
                MPI_Send(host_data,1,MPI_INT,1,0,MPI_COMM_WORLD);
                MPI_Recv(host_data,1,MPI_INT,1,0,MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
                queues[0].enqueueWriteBuffer(mem,CL_TRUE,0,sizeof(int),host_data);
            }
            if(rank==1)
            {
                MPI_Recv(host_data,1,MPI_INT,0,0,MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
                queues[0].enqueueWriteBuffer(mem,CL_TRUE,0,sizeof(int),host_data);

                queues[0].enqueueTask(kernels[0]);
                queues[0].finish();
                MPI_Send(host_data,1,MPI_INT,0,0,MPI_COMM_WORLD);

            }
        }
        if(rank==0)
        {
            endt=current_time_usecs();
            times.push_back(endt-startt);
        }



        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

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
        double data_sent_KB=KB;
        cout << "Average Latency (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "host_based_latency.dat";
        ofstream fout(filename.str());
        fout << "#Average Latencey (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Execution times (usecs):"<<endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();
    }

    CHECK_MPI(MPI_Finalize());

}
