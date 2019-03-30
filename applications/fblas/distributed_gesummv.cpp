/**
 *  ATTENTION: helpers modified to have better performance (volatile, N %64 ==0 ...)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cblas.h>
#include <math.h>

#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#define MPI
#if defined(MPI)
#include "../../include/utils/smi_utils.hpp"
#endif


using namespace std;
float *A,*B,*x,*y;
float *fpga_res_y;
void generate_float_matrix(float *A,int N,int M)
{
    for(int i=0;i<N;i++)
    {
        for(int j=0;j<M;j++)
           A[i*M+j] = i;//  static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/10.0));
    }
}

void generate_float_vector (float *x, int N)
{
    for(int i=0;i<N;i++)
        x[i] = 1;// static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1.0));
//        x[i]=1  ;
}



const double flteps = 1e-4;

inline bool test_equals(float result, float expected, float relative_error)
{
    //check also that the parameters are numbers
    return result==result && expected ==expected && ((result==0 && expected ==0) || fabs(result-expected)/fabs(expected) < relative_error);
}

int main(int argc, char *argv[])
{
    #if defined(MPI)
    CHECK_MPI(MPI_Init(&argc, &argv));
    #endif
    //command line argument parsing
    if(argc<15)
    {
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <n> -a <alpha> -c <beta> -r <num runs> -k <rank 0/1> -f <fpga>"<<endl;
        exit(-1);
    }

    int c;
    int n,m,runs,rank;
    float alpha,beta;
    std::string program_path,program_path_streamed;;
    std::string json_path;
    int fpga;

    while ((c = getopt (argc, argv, "n:b:r:a:c:k:f:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'a':
                alpha=atof(optarg);
                break;
            case 'c':
                beta=atof(optarg);
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
            case'k':
                {
                    rank=atoi(optarg);
                    if(rank!=0 && rank!=1)
                    {
                        cerr << "Error: rank may be 0-1"<<endl;
                        exit(-1);
                    }

                    break;
                }
            default:
                cerr << "Usage: "<< argv[0]<<""<<endl;
                exit(-1);
        }
    #if defined(MPI)
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    int rank;
    int fpga = rank % 2; // in this case is ok, pay attention
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cerr << "Program: " << program_path << std::endl;
    #endif


    //set affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
        cerr << "Cannot set thread to CPU " << 0<< endl;
    }

    //create data
    posix_memalign ((void **)&A, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*n*sizeof(float));
    posix_memalign ((void **)&B, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*n*sizeof(float));
    posix_memalign ((void **)&x, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));
    posix_memalign ((void **)&y, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));
    posix_memalign ((void **)&fpga_res_y, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));

    generate_float_vector(x,n);
    generate_float_matrix(A,n,n);
    generate_float_matrix(B,n,n);

    std::vector<double> streamed_times,transfer_times;

    cl::Platform platform;
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
            //we don't need to start all the CKs in this case

            kernel_names.push_back("gemv");
            kernel_names.push_back("axpy");
            kernel_names.push_back("kernel_read_matrix_A_0");
            kernel_names.push_back("kernel_read_vector_x_0");
            kernel_names.push_back("kernel_read_vector_y_0");
            kernel_names.push_back("kernel_write_vector_0");
            kernel_names.push_back("CK_S_0");
            kernel_names.push_back("CK_R_0");
            break;
        case 1:
            cout << "Starting rank 1 on FPGA "<<fpga<<endl;
            kernel_names.push_back("gemv");
            kernel_names.push_back("kernel_read_matrix_A_0");
            kernel_names.push_back("kernel_read_vector_x_0");
            kernel_names.push_back("kernel_read_vector_y_0");
            kernel_names.push_back("CK_S_0");
            kernel_names.push_back("CK_R_0");
            break;
    }
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names,kernels,queues);
    int tile_size=2048;
    cout << "Executing with tile size " << tile_size<<endl;
    //TODO fix this
    cl::Buffer input_x(context, CL_MEM_READ_ONLY|CL_CHANNEL_3_INTELFPGA, n *sizeof(float));
    cl::Buffer output_y(context, CL_MEM_READ_WRITE|CL_CHANNEL_4_INTELFPGA, n * sizeof(float));
    size_t elem_per_module=n*n/2;
    cl::Buffer input_M_0(context, CL_MEM_READ_ONLY|CL_CHANNEL_1_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer input_M_1(context, CL_MEM_READ_ONLY|CL_CHANNEL_2_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,2);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,1);
    if(rank==0)
    {
        //senders routing tables
        char rt_s_0[2]={1,2}; //this one is useless
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,2,rt_s_0);
        //receivers routing tables: these are meaningless
        char rt_r_0[1]={4}; //the first  is connect to application endpoint (TAG=0)
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,1,rt_r_0);

        //prepare data
        //copy the matrix interleaving it into two modules
        size_t offset=0;
        size_t increment=64/2*sizeof(float);
        const int loop_it=((int)(n))/64;   //W must be a divisor of M
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<loop_it;j++)
            {
                //write to the different banks
                queues[0].enqueueWriteBuffer(input_M_0, CL_FALSE,offset, (64/2)*sizeof(float),&(A[i*n+j*64]));
                queues[0].enqueueWriteBuffer(input_M_1, CL_FALSE,offset, (64/2)*sizeof(float),&(A[i*n+j*64+32]));

                offset+=increment;
            }
        }
        queues[0].finish();
        queues[0].enqueueWriteBuffer(input_x,CL_TRUE,0,n*sizeof(float),x);
        queues[0].finish();
        printf("Data copied\n");

        //gemv_A
        int one=1;
        int zero=0;
        float fzero=0,fone=1;
        int x_repetitions=ceil((float)(n)/tile_size);

        kernels[0].setArg(0, sizeof(int),&one);
        kernels[0].setArg(1, sizeof(int),&n);
        kernels[0].setArg(2, sizeof(int),&n);
        kernels[0].setArg(3, sizeof(float),&alpha);
        kernels[0].setArg(4, sizeof(float),&fzero);

        //axpy
        kernels[1].setArg(0, sizeof(float),&fone);
        kernels[1].setArg(1, sizeof(int),&n);

        //read_matrix_A
        kernels[2].setArg(0, sizeof(cl_mem),&input_M_0);
        kernels[2].setArg(1, sizeof(cl_mem),&input_M_1);
        kernels[2].setArg(2, sizeof(int),&n);
        kernels[2].setArg(3, sizeof(int),&n);
        kernels[2].setArg(4, sizeof(int),&n);


        //read_vector_x
        kernels[3].setArg(0, sizeof(cl_mem),&input_x);
        kernels[3].setArg(1, sizeof(int),&n);
        kernels[3].setArg(2, sizeof(int),&tile_size);
        kernels[3].setArg(3, sizeof(int),&x_repetitions);

        //read_vector_y
        kernels[4].setArg(0, sizeof(cl_mem),&output_y);
        kernels[4].setArg(1, sizeof(int),&zero);
        kernels[4].setArg(2, sizeof(int),&zero);
        kernels[4].setArg(3, sizeof(int),&zero);



        //write_vector
        kernels[5].setArg(0, sizeof(cl_mem),&output_y);
        kernels[5].setArg(1, sizeof(int),&n);
        kernels[5].setArg(2, sizeof(int),&tile_size);

        //set Arguments for CKS
        //args for the CK_Ss
        kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[6].setArg(1,sizeof(char),&rank_count);

        //args for the CK_Rs
        kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[7].setArg(1,sizeof(char),&rank);
        kernels[7].setArg(2,sizeof(char),&rank_count);
    }
    else
    {
        //senders routing tables
        char rt_s_0[2]={0,1}; //we go to I/O for sending to rank 0
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,2,rt_s_0);
        //receivers routing tables: these are meaningless
        char rt_r_0[1]={100}; //this one is useless
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,1,rt_r_0);


        //copy the matrix interleaving it into two modules
        size_t offset=0;
        size_t increment=64/2*sizeof(float);
        const int loop_it=((int)(n))/64;   //W must be a divisor of M
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<loop_it;j++)
            {
                //write to the different banks
                queues[0].enqueueWriteBuffer(input_M_0, CL_FALSE,offset, (64/2)*sizeof(float),&(B[i*n+j*64]));
                queues[0].enqueueWriteBuffer(input_M_1, CL_FALSE,offset, (64/2)*sizeof(float),&(B[i*n+j*64+32]));

                offset+=increment;
            }
        }
        queues[0].finish();
        queues[0].enqueueWriteBuffer(input_x,CL_TRUE,0,n*sizeof(float),x);
        queues[0].finish();

        //gemv_A
        int one=1;
        int zero=0;
        float fzero=0;
        int x_repetitions=ceil((float)(n)/tile_size);

        kernels[0].setArg(0, sizeof(int),&one);
        kernels[0].setArg(1, sizeof(int),&n);
        kernels[0].setArg(2, sizeof(int),&n);
        kernels[0].setArg(3, sizeof(float),&beta);
        kernels[0].setArg(4, sizeof(float),&fzero);


        //read_matrix_B
        kernels[1].setArg(0, sizeof(cl_mem),&input_M_0);
        kernels[1].setArg(1, sizeof(cl_mem),&input_M_1);
        kernels[1].setArg(2, sizeof(int),&n);
        kernels[1].setArg(3, sizeof(int),&n);
        kernels[1].setArg(4, sizeof(int),&n);


        //read_vector_x
        kernels[2].setArg(0, sizeof(cl_mem),&input_x);
        kernels[2].setArg(1, sizeof(int),&n);
        kernels[2].setArg(2, sizeof(int),&tile_size);
        kernels[2].setArg(3, sizeof(int),&x_repetitions);

        //read_vector_y useless
        kernels[3].setArg(0, sizeof(cl_mem),&output_y);
        kernels[3].setArg(1, sizeof(int),&zero);
        kernels[3].setArg(2, sizeof(int),&zero);
        kernels[3].setArg(3, sizeof(int),&zero);


        //set Arguments for CKS
        //args for the CK_Ss
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[4].setArg(1,sizeof(char),&rank_count);

        //args for the CK_Rs
        kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[5].setArg(1,sizeof(char),&rank);
        kernels[5].setArg(2,sizeof(char),&rank_count);



    }

    cout << "Ready to start " <<endl;
    //start the CK_S/CK_R
    const int num_kernels=kernel_names.size();
    queues[num_kernels-1].enqueueTask(kernels[num_kernels-1]);
    queues[num_kernels-2].enqueueTask(kernels[num_kernels-2]);

    // wait for other nodes
    #if defined(MPI)
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    #endif

    timestamp_t start=current_time_usecs();
    cl::Event events[6];


    //Program startup
    for(int i=0;i<runs;i++)
    {
        for(int i=0;i<num_kernels-2;i++)
            queues[i].enqueueTask(kernels[i],nullptr,&events[i]);
        for(int i=0;i<kernel_names.size()-2;i++)
            queues[i].finish();
    }
    //Program ends
    timestamp_t end=current_time_usecs();
    // wait for other nodes
    #if defined(MPI)
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    #else
    sleep(2);
    #endif

      //compute execution time using OpenCL profiling
    ulong min_start=4294967295, max_end=0;
    ulong end;
    ulong start;
    for(int i=0;i<kernel_names.size();i++)
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


    if(rank ==0)
    {
         queues[0].enqueueReadBuffer(output_y,CL_FALSE,0,n*sizeof(float),fpga_res_y);
        //check
        timestamp_t comp_start=current_time_usecs();
        cblas_sgemv(CblasRowMajor,CblasNoTrans,n,n,beta,B,n,x,1,0,y,1);
        cblas_sgemv(CblasRowMajor,CblasNoTrans,n,n,alpha,A,n,x,1,1,y,1);
        timestamp_t cpu_time=current_time_usecs()-comp_start;


        //forse meglio passare a norma
        bool ok=true;

        for(int i=0;i<n;i++)
        {
            if(!test_equals(fpga_res_y[i],y[i],flteps))
            {
                ok=false;
                cout << i << " Error streamed: " <<fpga_res_y[i]<<" != " <<y[i]<<endl;
            }
        }


        if(ok )
            std::cout <<"OK!!!" <<std::endl;
        else
            std::cout <<"ERROR!!!" <<std::endl;

        cout << "Computation time (usecs): " <<(double)(end-start)/runs <<endl;
        cout << "Computation time with rpof: "<< (double)((max_end-min_start)/1000.0f)<<endl;
    }
    #if defined(MPI)
    CHECK_MPI(MPI_Finalize());
    #endif

}
