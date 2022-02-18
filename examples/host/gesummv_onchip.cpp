/**
 *  GESUMMV, on chip version
 *  The results of the two gesummv are streamed to the axpy
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cblas.h>
#include <math.h>
#include <fstream>
#include <utils/ocl_utils.hpp>
#include <utils/utils.hpp>
#define TILE_SIZE 128 //define this as used in the opencl kernel

#if !defined(CL_CHANNEL_1_INTELFPGA)
// include this header if channel macros are not defined in cl.hpp (versions >=19.0)
#include "CL/cl_ext_intelfpga.h"
#endif

using namespace std;
float *A,*B,*x,*y;
float *fpga_res_y;
//#define BLOCKING

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
        x[i] = i;// static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/1.0));
//        x[i]=1  ;
}


const double flteps = 1e-4;

inline bool test_equals(float result, float expected, float relative_error)
{
    //check also that the parameters are numbers
    return result==result && expected ==expected && ((result==0 && expected ==0) || fabs(result-expected)/fabs(expected) < relative_error);
}


void testStreamed(std::string program_path,int n, int m, float alpha, float beta, std::vector<double> &times, std::vector <double> &transfer_times,int runs )
{
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<std::string> kernel_names={"gemv_A","gemv_B", "axpy", "read_matrix_A", "read_matrix_B", "read_vector_x_A",  "read_vector_y_A", "read_vector_x_B", "read_vector_y_B", "write_vector"};
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    IntelFPGAOCLUtils::initEnvironment(platform,device,0,context,program,program_path,kernel_names,kernels,queues);
    //create data
    posix_memalign ((void **)&fpga_res_y, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));

    cl::Buffer input_x(context, CL_MEM_READ_ONLY|CL_CHANNEL_1_INTELFPGA, m *sizeof(float));
    cl::Buffer input_output_y(context, CL_MEM_READ_WRITE|CL_CHANNEL_2_INTELFPGA, n * sizeof(float));
    cl::Buffer input_A(context, CL_MEM_READ_ONLY|CL_CHANNEL_3_INTELFPGA, n * m*sizeof(float));
    cl::Buffer input_B(context, CL_MEM_WRITE_ONLY|CL_CHANNEL_4_INTELFPGA, n * m*sizeof(float));

     //set kernel arguments
    int one=1;
    int zero=0;
    float fzero=0;
    float fone=1;
    int width=32;
    int tile_size=TILE_SIZE;  //attention, change this if necessary
    std::cout << "Executing streamed version with width: "<<width << "and tile "<<tile_size<<endl;
    int x_repetitions=ceil((float)(n)/tile_size);

    //gemv_A
    kernels[0].setArg(0, sizeof(int),&one);
    kernels[0].setArg(1, sizeof(int),&n);
    kernels[0].setArg(2, sizeof(int),&m);
    kernels[0].setArg(3, sizeof(float),&alpha);
    kernels[0].setArg(4, sizeof(float),&fzero);

    //gemv_B
    kernels[1].setArg(0, sizeof(int),&one);
    kernels[1].setArg(1, sizeof(int),&n);
    kernels[1].setArg(2, sizeof(int),&m);
    kernels[1].setArg(3, sizeof(float),&beta);
    kernels[1].setArg(4, sizeof(float),&fzero);

    //axpy
    kernels[2].setArg(0, sizeof(float),&fone);
    kernels[2].setArg(1, sizeof(int),&n);

    //read_matrix_A
    kernels[3].setArg(0, sizeof(cl_mem),&input_A);
    kernels[3].setArg(1, sizeof(int),&n);
    kernels[3].setArg(2, sizeof(int),&m);
    kernels[3].setArg(3, sizeof(int),&m);

     //read_matrix_B
    kernels[4].setArg(0, sizeof(cl_mem),&input_B);
    kernels[4].setArg(1, sizeof(int),&n);
    kernels[4].setArg(2, sizeof(int),&m);
    kernels[4].setArg(3, sizeof(int),&m);

    //read_vector_x_A
    kernels[5].setArg(0, sizeof(cl_mem),&input_x);
    kernels[5].setArg(1, sizeof(int),&m);
    kernels[5].setArg(2, sizeof(int),&tile_size);
    kernels[5].setArg(3, sizeof(int),&x_repetitions);

     //read_vector_y_A
    kernels[6].setArg(0, sizeof(cl_mem),&input_output_y);
    kernels[6].setArg(1, sizeof(int),&n);
    kernels[6].setArg(2, sizeof(int),&tile_size);
    kernels[6].setArg(3, sizeof(int),&zero);

    //read_vector_x_B
    kernels[7].setArg(0, sizeof(cl_mem),&input_x);
    kernels[7].setArg(1, sizeof(int),&m);
    kernels[7].setArg(2, sizeof(int),&tile_size);
    kernels[7].setArg(3, sizeof(int),&x_repetitions);

    //read_vector_y_B
    kernels[8].setArg(0, sizeof(cl_mem),&input_output_y);
    kernels[8].setArg(1, sizeof(int),&n);
    kernels[8].setArg(2, sizeof(int),&tile_size);
    kernels[8].setArg(3, sizeof(int),&zero);

    //write_vector
    kernels[9].setArg(0, sizeof(cl_mem),&input_output_y);
    kernels[9].setArg(1, sizeof(int),&n);
    kernels[9].setArg(2, sizeof(int),&tile_size);

    timestamp_t transfer_time;

    for(int i=0;i<runs;i++)
    {
        cl::Event events[10];
        
        timestamp_t comp_start=current_time_usecs();
        queues[0].enqueueWriteBuffer(input_A,CL_FALSE,0,n*m*sizeof(float),A);
        queues[0].enqueueWriteBuffer(input_B,CL_FALSE,0,n*m*sizeof(float),B);
        queues[0].enqueueWriteBuffer(input_x,CL_FALSE,0,m*sizeof(float),x);
        queues[0].finish();
        transfer_time=current_time_usecs()-comp_start;
        asm volatile("": : :"memory");

        comp_start=current_time_usecs();
        asm volatile("": : :"memory");
        for(int i=0;i<kernel_names.size();i++)
            queues[i].enqueueNDRangeKernel(kernels[i],cl::NullRange,cl::NDRange(1));
        for(int i=0;i<kernel_names.size();i++)
            queues[i].finish();

        asm volatile("": : :"memory");
        timestamp_t comp_t=current_time_usecs()-comp_start;
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
        times.push_back((double)((max_end-min_start)/1000.0f));
        comp_start=current_time_usecs();
        queues[0].enqueueReadBuffer(input_output_y,CL_FALSE,0,n*sizeof(float),fpga_res_y);
        queues[0].finish();
        transfer_time+=current_time_usecs()-comp_start;
        transfer_times.push_back(transfer_time);

    }

}

int main(int argc, char *argv[])
{

    //command line argument parsing
    if(argc<13)
    {
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <n> -m <m> -a <alpha> -c <beta> -r <num runs>"<<endl;
        exit(-1);
    }

    int c;
    int n,m,runs;
    double alpha,beta;
    std::string program_path,program_path_streamed;;
    std::string json_path;
    while ((c = getopt (argc, argv, "n:b:r:a:c:m:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'm':
                m=atoi(optarg);
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

            case 'r':
                runs=atoi(optarg);
                break;
            default:
                cerr << "Usage: "<< argv[0]<<""<<endl;
                exit(-1);
        }

    //set affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
        cerr << "Cannot set thread to CPU " << 0<< endl;
    }

    //create data
    posix_memalign ((void **)&A, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*m*sizeof(float));
    posix_memalign ((void **)&B, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*m*sizeof(float));
    posix_memalign ((void **)&x, IntelFPGAOCLUtils::AOCL_ALIGNMENT, m*sizeof(float));
    posix_memalign ((void **)&y, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));
    generate_float_vector(x,m);
    generate_float_matrix(A,n,m);
    generate_float_matrix(B,n,m);

    std::vector<double> streamed_times,transfer_times;


    //check
    timestamp_t comp_start=current_time_usecs();
    cblas_sgemv(CblasRowMajor,CblasNoTrans,n,m,beta,B,m,x,1,0,y,1);
    cblas_sgemv(CblasRowMajor,CblasNoTrans,n,m,alpha,A,m,x,1,1,y,1);
    timestamp_t cpu_time=current_time_usecs()-comp_start;


    //forse meglio passare a norma
    bool ok=true;


    testStreamed(program_path,n,m,alpha,beta,streamed_times, transfer_times,runs);

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

    //compute the average and standard deviation of times
    double streamed_mean=0;
    for(auto t:streamed_times)
        streamed_mean+=t;
    streamed_mean/=runs;


    double streamed_stddev=0;
    for(auto t:streamed_times)
        streamed_stddev+=((t-streamed_mean)*(t-streamed_mean));
    streamed_stddev=sqrt(streamed_stddev/(runs-1));

    //compute average and standard deviation of transfer times
    double transfer_mean=0, transfer_stddev=0;
    for(auto t:transfer_times)
        transfer_mean+=t;
    transfer_mean/=runs;
    for(auto t:transfer_times)
        transfer_stddev+=((t-transfer_mean)*(t-transfer_mean));
    transfer_stddev=sqrt(transfer_stddev/(runs-1));

    //double streamed_conf_interval_95=1.96*streamed_stddev/sqrt(runs);
    double streamed_conf_interval_99=2.58*streamed_stddev/sqrt(runs);
    //double streamed_conf_interval_95=1.96*streamed_stddev/sqrt(runs);
    double transfer_conf_interval_99=2.58*transfer_stddev/sqrt(runs);

    cout << "Computation time over cpu (usecs): "<<cpu_time<<endl;
    cout << "Transfer time mesured with streamed (usec) " << transfer_mean << " (sttdev: " << transfer_stddev<<")"<<endl;
    cout << "Computation time over fpga with streamed (usecs): "<<streamed_mean<< " (sttdev: " << streamed_stddev<<")"<<endl;
    cout << "streamed Conf interval 99: "<<streamed_conf_interval_99<<endl;
    cout << "streamed Conf interval 99 within " <<(streamed_conf_interval_99/streamed_mean)*100<<"% from mean" <<endl;

    //save the info into output files
    ofstream fout("streamed_output.dat");
    fout << "#N = "<<n<<", Runs = "<<runs<<endl;
    fout << "#Average Computation time (usecs): "<<streamed_mean<<endl;
    fout << "#Standard deviation (usecs): "<<streamed_stddev<<endl;
    fout << "#Confidence interval 99%: +- "<<streamed_conf_interval_99<<endl;
    fout << "#Execution times (usecs):"<<endl;
    for(auto t:streamed_times)
        fout << t << endl;
    fout.close();



    fout.open("transfer_output.dat");
    fout << "#N = "<<n<<", Runs = "<<runs<<endl;
    fout << "#Average Transfer Time(usecs): "<<transfer_mean<<endl;
    fout << "#Standard deviation (usecs): "<<transfer_stddev<<endl;
    fout << "#Transfer times (usecs):"<<endl;
    fout << "#Confidence interval 99%: +- "<<transfer_conf_interval_99<<endl;

    for(auto t:transfer_times)
        fout << t << endl;
    fout.close();

}
