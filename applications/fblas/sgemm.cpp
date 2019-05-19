/**
 * Matrix matrix multiplication test HLS modules
 */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "CL/opencl.h"
#include <fstream>
#include <cmath>
#include <cblas.h>

#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"

#define CHECK
void generate_float_matrix(float *A,int N,int M)
{
    for(int i=0;i<N;i++)
    {
        for(int j=0;j<M;j++)
           A[i*M+j] = i;//  static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/10.0));
    }
}

const double flteps = 1e-4;

inline bool test_equals(float result, float expected, float relative_error)
{
    //check also that the parameters are numbers
    return result==result && expected ==expected && ((result==0 && expected ==0) || fabs(result-expected)/fabs(expected) < relative_error);
}

using namespace std;
int main(int argc, char *argv[])
{
    CHECK_MPI(MPI_Init(&argc, &argv));
    //command line argument parsing
    if(argc<9)
    {
        cerr << "Matrix multiplication A*B where A is NxP and B is KxM " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -p <length> -m <length>"<<endl;
        exit(-1);
    }
    const unsigned int align=IntelFPGAOCLUtils::AOCL_ALIGNMENT;
    int c;
    int n,m,p;
    bool double_precision;
    std::string program_path;
    int rank;
    int runs=1;
    while ((c = getopt (argc, argv, "n:m:p:b:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'm':
                m=atoi(optarg);
                break;
            case 'p':
                p=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;

            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length> -p <length> -m <length> "<<endl;
                exit(-1);
        }

    cout << "Performing multiplication A ("<<n<<" x "<< p<<") * B ("<<p<<" x "<<m<<")"<<endl;

    //init data: matrices should be memorized as flat arrays (useful for kernel execution)
    float *A,*B,*C,*fpgaC;
    float alpha=1.0f;
    float beta=1.0f;
#if defined(CHECK)

    posix_memalign ((void **)&A, align, n*p*sizeof(float));
    posix_memalign ((void **)&B, align, p*m*sizeof(float));
    posix_memalign ((void **)&C, align, n*m*sizeof(float));
    posix_memalign ((void **)&fpgaC, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*m*sizeof(float));
    generate_float_matrix(A,n,p);
    generate_float_matrix(B,p,m);
    generate_float_matrix(C,n,m);
#endif
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    int fpga = rank % 2; // in this case is ok, pay attention
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cerr << "Program: " << program_path << std::endl;

    cl_int status;
    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"read_matrix_A","read_matrix_B","sgemm","write_matrix"};
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    cl::Buffer input_A(context,CL_MEM_READ_ONLY|CL_CHANNEL_1_INTELFPGA,n*p*sizeof(float));
    cl::Buffer input_B(context,CL_MEM_READ_ONLY|CL_CHANNEL_2_INTELFPGA,p*m*sizeof(float));
    cl::Buffer output_C(context,CL_MEM_READ_WRITE|CL_CHANNEL_3_INTELFPGA,n*m*sizeof(float));
    queues[0].enqueueWriteBuffer(input_A,CL_TRUE,0,n*p*sizeof(float),A);
    queues[0].enqueueWriteBuffer(input_B,CL_TRUE,0,p*m*sizeof(float),B);
    queues[0].enqueueWriteBuffer(output_C,CL_TRUE,0,n*m*sizeof(float),C);

    //read_matrix A
    kernels[0].setArg(0,sizeof(cl_mem),&input_A);
    kernels[0].setArg(1,sizeof(int),&n);
    kernels[0].setArg(2,sizeof(int),&p);
    kernels[0].setArg(3,sizeof(int),&m);
    kernels[0].setArg(4,sizeof(int),&p);

    //read_matrix B
    kernels[1].setArg(0,sizeof(cl_mem),&input_B);
    kernels[1].setArg(1,sizeof(int),&n);
    kernels[1].setArg(2,sizeof(int),&p);
    kernels[1].setArg(3,sizeof(int),&m);
    kernels[1].setArg(4,sizeof(int),&m);

    //sgemm
    kernels[2].setArg(0,sizeof(int),&n);
    kernels[2].setArg(1,sizeof(int),&m);
    kernels[2].setArg(2,sizeof(int),&p);
    kernels[2].setArg(3,sizeof(float),&alpha);

    //write results
    kernels[3].setArg(0,sizeof(cl_mem),&output_C);
    kernels[3].setArg(1,sizeof(float),&beta);
    kernels[3].setArg(2,sizeof(int),&n);
    kernels[3].setArg(3,sizeof(int),&m);
    kernels[3].setArg(4,sizeof(int),&m);


    std::vector<double> times;

     for(int r=0;r<runs;r++)
    {
        cl::Event events[4];
        for(int i=0;i<kernels.size();i++)
            queues[i].enqueueTask(kernels[i],nullptr,&events[i]);

        //wait
        for(int i=0;i<kernels.size();i++)
            queues[i].finish();

        ulong min_start=4294967295, max_end=0;
        ulong end;
        ulong start;
        for(int i=0;i<kernels.size();i++)
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
    }


#if defined(CHECK)
    queues[0].enqueueReadBuffer(output_C,CL_TRUE,0,n*m*sizeof(float),fpgaC);
    cblas_sgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,n,m,p,alpha,A,p,B,m,alpha,C,m);
    const double flteps = 1e-4;
    bool ok=true;

    for(int i=0;i<n;i++)
    {
        for(int j=0;j<m;j++)
        {
            if(!test_equals(fpgaC[i*m+j],C[i*m+j],flteps))
            {
                std::cout <<"["<<i<<", "<<j<<" ] "<<fpgaC[i*m+j]<<"\t"<<C[i*m+j]<<std::endl;
                ok=false;
            }
        }
    }
    if(ok) std::cout << "Computation Ok!" <<std::endl;
    else std::cout << "ERRORR!!!!!!!" << std::endl;

#endif

    //compute the average and standard deviation of times
    double mean=0;
    for(auto t:times)
        mean+=t;
    mean/=runs;
    //report the mean in usecs

    double stddev=0;
    for(auto t:times)
        stddev+=((t-mean)*(t-mean));
    stddev=sqrt(stddev/runs);

    double computation_gops=((double)(m)*n*(2*p-1)+m*n)/1000000000;
    double measured_gops=computation_gops/((mean)/1000000.0);
    cout << std::fixed;
    cout << "FPGA Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
    std::cout << "FPGA GOps/s: " << measured_gops<<std::endl;

    //save the info into output file
    ofstream fout("output.dat");
    fout << "#N = "<<n<<", Runs = "<<runs<<endl;
    fout << std::fixed;
    fout << "#Average Computation time (usecs): "<<mean<<endl;
    fout << "#Standard deviation (usecs): "<<stddev<<endl;
    fout << "#GOPS/s: "<<measured_gops<<endl;
    fout << "#Execution times (usecs):"<<endl;
    for(auto t:times)
        fout << t << endl;
    fout.close();

    CHECK_MPI(MPI_Finalize());


}
