

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"

//#define CHECK
using namespace std;


void generate_float_matrix(float *A,int N,int M)
{
    for(int i=0;i<N;i++)
    {
        for(int j=0;j<M;j++)
           A[i*M+j] = i;
    }
}

int main(int argc, char *argv[])
{

    //command line argument parsing
    if(argc<7)
    {
        cerr << "Read Matrix " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length>  -m <length> "<<endl;
        exit(-1);
    }
    int n,m;
    int c;
    std::string program_path;
    while ((c = getopt (argc, argv, "n:b:m:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
             case 'm':
             {
                m=atoi(optarg);
                if(m%64 !=0)
                {
                    cerr << "M must be a multiple of 64"<<endl;
                    exit(-1);
                }
                break;
            }
            case 'b':
                program_path=std::string(optarg);
                break;
            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cout << "Performing read matrix test with size "<<n<<" x " << m << " ("<<(n*m*sizeof(float))/1024.0/1024.0<< " MB) "<<endl;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"read_matrix_A", "receiver"};

    IntelFPGAOCLUtils::initEnvironment(platform,device,0,context,program,program_path,kernel_names, kernels,queues);

    float *matrix;
    float res;
    //create memory buffers
    posix_memalign ((void **)&matrix, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*m*sizeof(float));
    size_t elem_per_module=n*m/4;
    cl::Buffer input_A_0(context, CL_MEM_READ_ONLY|CL_CHANNEL_1_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer input_A_1(context, CL_MEM_READ_ONLY|CL_CHANNEL_2_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer input_A_2(context, CL_MEM_READ_ONLY|CL_CHANNEL_3_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer input_A_3(context, CL_MEM_READ_ONLY|CL_CHANNEL_4_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer sum(context,CL_MEM_WRITE_ONLY,sizeof(float));
    generate_float_matrix(matrix,n,m);


    const int loop_it=((int)(m))/64;   //W must be a divisor of M
    size_t offset=0;
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<loop_it;j++)
        {
            //write to the different banks
            queues[0].enqueueWriteBuffer(input_A_0, CL_FALSE,offset, 16*sizeof(float),&(matrix[i*m+j*64]));
            queues[0].enqueueWriteBuffer(input_A_1, CL_FALSE,offset, 16*sizeof(float),&(matrix[i*m+j*64+16]));
            queues[0].enqueueWriteBuffer(input_A_2, CL_FALSE,offset, 16*sizeof(float),&(matrix[i*m+j*64+32]));
            queues[0].enqueueWriteBuffer(input_A_3, CL_FALSE,offset, 16*sizeof(float),&(matrix[i*m+j*64+48]));
            offset+=16*sizeof(float);
            queues[0].finish();
        }
    }
        //args for the apps
    kernels[0].setArg(0,sizeof(cl_mem),&input_A_0);
    kernels[0].setArg(1,sizeof(cl_mem),&input_A_1);
    kernels[0].setArg(2,sizeof(cl_mem),&input_A_2);
    kernels[0].setArg(3,sizeof(cl_mem),&input_A_3);
    kernels[0].setArg(4,sizeof(int),&n);
    kernels[0].setArg(5,sizeof(int),&m);
    kernels[0].setArg(6,sizeof(int),&m);
    kernels[1].setArg(0,sizeof(cl_mem),&sum);
    kernels[1].setArg(1,sizeof(int),&n);
    kernels[1].setArg(2,sizeof(int),&m);


    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size();i++)
        queues[i].enqueueTask(kernels[i]);
    for(int i=0;i<kernel_names.size();i++)
        queues[i].finish();

    timestamp_t end=current_time_usecs();
    queues[0].enqueueReadBuffer(sum,CL_TRUE,0,sizeof(float),&res);

    float real_res=0;
    for(int i=0;i<n;i++)
        for(int j=0;j<m;j++)
            real_res+=matrix[i*m+j];

    if(res==real_res)
        cout << "OK!"<<endl;
    else
        cout << "ERROR! "<< real_res << " != " << res<< endl;

    cout << "Time elapsed (usecs): "<<end-start<<endl;
    double data_bytes=(n*m)*sizeof(float);
    double mem_bandw=((double)data_bytes/((end-start)/1000000.0))/(1024*1024*1024); //GB/s
    cout << "Memory Bandwidth (GB/s): "<<mem_bandw<<endl;
}
