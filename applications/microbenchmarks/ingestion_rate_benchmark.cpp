/**
    Ingestion benchmark

 */

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cmath>
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
    int n;
    int c;
    std::string program_path;
    int rank;
    int fpga,runs;
    while ((c = getopt (argc, argv, "n:b:r:f:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'f':
                fpga=atoi(optarg);
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
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << std::endl;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    switch(rank)
    {
        case 0:
            cout << "Starting rank 0 on FPGA "<<fpga<<endl;
            kernel_names.push_back("app_0");
            //we don't need to start all CK_S/CK_R if we don't use them

            break;
        case 1:
            cout << "Starting rank 1 on FPGA "<<fpga<<endl;
            kernel_names.push_back("app_1");
            kernel_names.push_back("app_2");
            break;
    }
    kernel_names.push_back("CK_S_0");
    kernel_names.push_back("CK_S_1");
    kernel_names.push_back("CK_S_2");
    kernel_names.push_back("CK_S_3");
    kernel_names.push_back("CK_R_0");
    kernel_names.push_back("CK_R_1");
    kernel_names.push_back("CK_R_2");
    kernel_names.push_back("CK_R_3");

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char ranks=2;
    char tags=2;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);
    if(rank==0)
    {
        //senders routing tables
        char rt_s_0[2]={1,0};
        char rt_s_1[2]={1,2};   //Use 2 in rt_s_1 to send using QSFP0, use 4 to send with QSFP3
        char rt_s_2[2]={100,100}; //useless
        char rt_s_3[2]={1,0};   //if it receives something for rank 1 it send it in I/0
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,ranks,rt_s_2);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,ranks,rt_s_3);
        //receivers routing tables: these are meaningless
        char rt_r_0[2]={100,100}; //the first doesn't do anything
        char rt_r_1[2]={100,100};   //
        char rt_r_2[2]={100,100};
        char rt_r_3[2]={100,100};
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,rt_r_1);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,rt_r_2);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,rt_r_3);
        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);
        //args for the CK_Ss
        kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);

        //args for the CK_Rs
        kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[5].setArg(1,sizeof(char),&rank);
        kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[6].setArg(1,sizeof(char),&rank);
        kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
        kernels[7].setArg(1,sizeof(char),&rank);
        kernels[8].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
        kernels[8].setArg(1,sizeof(char),&rank);

    }
    if(rank==1)
    {
        //sender routing tables are meaningless
        char rt_s_0[2]={0,0};
        char rt_s_1[2]={100,100};
        char rt_s_2[2]={100,100};
        char rt_s_3[2]={100,100};
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,ranks,rt_s_2);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,ranks,rt_s_3);
        //receivers routing tables
        char rt_r_0[2]={4,1}; //TAG 0 -> attached application, TAG 1 -> CK_R_1
        char rt_r_1[2]={100,4};
        char rt_r_2[2]={100,4};
        char rt_r_3[2]={100,2};//TAG 1-> CK_R_1
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,rt_r_1);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,rt_r_2);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,rt_r_3);

        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(int),&n);

        //args for the CK_Ss
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
        kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);

        //args for the CK_Rs
        kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[6].setArg(1,sizeof(char),&rank);
        kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[7].setArg(1,sizeof(char),&rank);
        kernels[8].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
        kernels[8].setArg(1,sizeof(char),&rank);
        kernels[9].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
        kernels[9].setArg(1,sizeof(char),&rank);

    }

    const int num_kernels=kernel_names.size();
    for(int i=num_kernels-1;i>=num_kernels-8;i--)
        queues[i].enqueueTask(kernels[i]);

    std::vector<double> times;

    for(int i=0;i<runs;i++)
    {


        cl::Event events[1]; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        //ATTENTION: If you are executing on the same host
        //since the PCIe is shared that could be problems in taking times
        //This mini sleep should resolve
        if(rank==0)
            usleep(10000);

        timestamp_t start=current_time_usecs();
        //TODO fix this
        /*
            Normal version
        for(int i=0;i<kernel_names.size()-8;i++)
            queues[i].enqueueTask(kernels[i],nullptr,&events[i]);

        for(int i=0;i<kernel_names.size()-8;i++)
            queues[i].finish();
        */
        //single QSFP version
        queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
        queues[0].finish();
        timestamp_t end=current_time_usecs();

        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
        if(rank==0)
        {
            ulong end, start;
            events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
            events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);

            double time= (double)((end-start)/1000.0f);
            times.push_back(time);
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
        double data_sent_KB=KB;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;

        //save the info into output file
        std::ostringstream filename;
        filename << "ingestion_rate_" << n << ".dat";
        std::cout << "Saving info into: "<<filename.str()<<std::endl;
        ofstream fout(filename.str());
        fout << "#Ingestion rate benchnmark: sending  "<<n<<" messages of size 1" <<std::endl;
        fout << "#Average Computation time (usecs): "<<mean<<endl;
        fout << "#Standard deviation (usecs): "<<stddev<<endl;
        fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
        fout << "#Execution times (usecs):"<<endl;
        for(auto t:times)
            fout << t << endl;
        fout.close();
    }
    CHECK_MPI(MPI_Finalize());

}
