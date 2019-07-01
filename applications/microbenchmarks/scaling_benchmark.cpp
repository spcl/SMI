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
#include "../../include/utils/smi_utils.hpp"
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
    //n=40;
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
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    kernel_names.push_back("app");
    kernel_names.push_back("app_1");
    kernel_names.push_back("smi_kernel_cks_0");
    kernel_names.push_back("smi_kernel_cks_1");
    kernel_names.push_back("smi_kernel_cks_2");
    kernel_names.push_back("smi_kernel_cks_3");
    kernel_names.push_back("smi_kernel_ckr_0");
    kernel_names.push_back("smi_kernel_ckr_1");
    kernel_names.push_back("smi_kernel_ckr_2");
    kernel_names.push_back("smi_kernel_ckr_3");


    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    const char tags=2;
    std::cout << "Version with " <<(int)tags<< " tags" << std::endl;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags*2); //NEW
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags*2);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags*2);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags*2);
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer check2(context,CL_MEM_WRITE_ONLY,1);

    //load routing tables
    char routing_tables_ckr[4][tags*2]; //only one tag
    char routing_tables_cks[4][rank_count];
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, tags*2, ROUTING_DIR, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
    }

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    /*sleep(rank);
    std::cout << "CK R Routing tables for rank "<<rank<<std::endl;
    for(int i = 0; i < kChannelsPerRank; ++i) {
        std::cout << "CK_R_" <<i<<std::endl;
        for (int ii = 0; ii < 2; ii++)
        {
            std::cout << "\tPort: "<<ii<<": ";
            std::cout << (int)routing_tables_ckr[i][ii*2] <<", " <<(int)routing_tables_ckr[i][ii*2+1]<<endl;
        }
    }*/

    //NEW TEMP:
    //duplicate the content of the CKR, will be sufficient for now
    /*  for (int i = 0; i < kChannelsPerRank; ++i) {
         for(int j=tags-1;j>=0;j--)
         {
             routing_tables_ckr[i][j*2]=routing_tables_ckr[i][j];
             routing_tables_ckr[i][j*2+1]=routing_tables_ckr[i][j];
         }

    }*/
    /*
    printf("Copied\n");
    for(int i=0;i<4;i++)
       std::cout << "CKR"<<i<<": "<<(int)routing_tables_ckr[i][0]<<", "<<(int)routing_tables_ckr[i][1]<<", "<<(int)routing_tables_ckr[i][2]<<", "<<(int)routing_tables_ckr[i][3]<<std::endl;
    */
    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags*2,&routing_tables_ckr[0][0]); //NEW
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags*2,&routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags*2,&routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags*2,&routing_tables_ckr[3][0]);

    if(rank==0)
    {
        char dest=(char)recv_rank;
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[0].setArg(1,sizeof(char),&dest);
        kernels[1].setArg(0,sizeof(int),&n);
        kernels[1].setArg(1,sizeof(char),&dest);
    }
    else
    {
        kernels[0].setArg(0,sizeof(cl_mem),&check);
        kernels[0].setArg(1,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(cl_mem),&check2);
        kernels[1].setArg(1,sizeof(int),&n);
    }
    //args for the CK_Ss
    kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[2].setArg(1,sizeof(char),&rank_count);
    kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[3].setArg(1,sizeof(char),&rank_count);
    kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[4].setArg(1,sizeof(char),&rank_count);
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);
    kernels[5].setArg(1,sizeof(char),&rank_count);

    //args for the CK_Rs
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[6].setArg(1,sizeof(char),&rank);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[7].setArg(1,sizeof(char),&rank);
    kernels[8].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[8].setArg(1,sizeof(char),&rank);
    kernels[9].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[9].setArg(1,sizeof(char),&rank);

    //start the CKs
    const int num_kernels=kernel_names.size();
    for(int i=num_kernels-1;i>=num_kernels-8;i--)
        queues[i].enqueueTask(kernels[i]);
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
