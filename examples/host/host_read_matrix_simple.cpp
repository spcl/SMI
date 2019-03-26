

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
           A[i*M+j] = i*M+j;
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

    cout << "Performing send/receive test with "<<n<<" integers"<<endl;

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
    cl::Buffer input_A(context, CL_MEM_READ_ONLY, n *m*sizeof(float));
    cl::Buffer sum(context,CL_MEM_WRITE_ONLY,sizeof(float));
    generate_float_matrix(matrix,n,m);

    queues[0].enqueueWriteBuffer(input_A, CL_TRUE,0, n *m*sizeof(float),matrix);
       
        //args for the apps
    kernels[0].setArg(0,sizeof(cl_mem),&input_A);
    kernels[0].setArg(1,sizeof(int),&n);
    kernels[0].setArg(2,sizeof(int),&m);
    kernels[0].setArg(3,sizeof(int),&m);
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
        cout << "ERROR!"<<endl;

    cout << "Time elapsed (usecs): "<<end-start<<endl;
}