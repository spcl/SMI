/**
    Reduce benchmark, host based:
    - each rank prepare data  (written in memory)
    - then it is copied into host and reduce to all the others
    - then the root will have the result copied into DRAM and read again

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
        cerr << "Reduce (integer) tester. Root is rank 0 " <<endl;
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
    cout << "Performing reduce test with "<<n<<" elements"<<endl;
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
   /* if(rank==0)
        program_path = replace(program_path, "<type>", std::string("root"));
    else
        program_path = replace(program_path, "<type>", std::string("rank"));*/
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
    std::vector<std::string> kernel_names={"app_rank","app_root"};


    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    int *host_data,*reduced_data;
    //ATTENTION: declare all the memory here otherwise it hangs
    cl::Buffer mem(context,CL_MEM_READ_WRITE,max(n,1)*sizeof(int));
    posix_memalign ((void **)&host_data, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(int));
    posix_memalign ((void **)&reduced_data, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(int));


    kernels[0].setArg(0,sizeof(cl_mem),&mem);
    kernels[0].setArg(1,sizeof(int),&n);
    kernels[1].setArg(0,sizeof(cl_mem),&mem);
    kernels[1].setArg(1,sizeof(int),&n);

    std::vector<double> times;
    timestamp_t startt,endt;
    //Program startup
    for(int i=0;i<runs;i++)
    {
        cl::Event events[3]; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        startt=current_time_usecs();
        queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
        queues[0].finish();
        //copy the data to host
        queues[0].enqueueReadBuffer(mem,CL_TRUE,0,n*sizeof(int),host_data);
        //reduce it
        MPI_Reduce(host_data,reduced_data,n,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);

        if(rank==0)
        {
            //copy to device and execute
            queues[1].enqueueWriteBuffer(mem,CL_TRUE,0,n*sizeof(int),reduced_data);
            queues[1].enqueueTask(kernels[1],nullptr,&events[1]);
            queues[1].finish();
            endt=current_time_usecs();
            times.push_back(endt-startt);
        }


        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    }
    if(rank==0) //compute the time on the root
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
        double data_sent_KB=KB;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "host_based_reduce_" << n << ".dat";
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
