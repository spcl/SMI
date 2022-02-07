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
#include <utils/ocl_utils.hpp>
#include <utils/utils.hpp>

#include "smi_generated_host.c"
#define ROUTING_DIR "smi-routes/"

#if !defined(CL_CHANNEL_1_INTELFPGA)
// include this header if channel macros are not defined in cl.hpp (versions >=19.0)
#include "CL/cl_ext_intelfpga.h"
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
    system("rm emulated_chan* 2> /dev/null;"); //remove old emulation channels
    CHECK_MPI(MPI_Init(&argc, &argv));
    //command line argument parsing
    if(argc<11)
    {
        cerr << "Usage: "<< argv[0]<<" -t <emulator/hardware>  -n <n>  -m <m> -a <alpha> -c <beta> -r <num runs> [-b \"<binary file with <rank> for the rank and <type> for the program>\"]"<<endl;
        exit(-1);
    }

    int c;
    int n,m,runs,rank;
    float alpha,beta;
    std::string program_path;
    int fpga;
    bool emulator;
    bool binary_file_provided=false;
    while ((c = getopt (argc, argv, "n:b:r:a:c:m:t:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'm':
                m=atoi(optarg);
                break;
            case 't':
            {
                std::string mode=std::string(optarg);
                if(mode!="emulator" && mode!="hardware")
                {
                    cerr << "Mode: emulator or hardware"<<endl;
                    exit(-1);
                }
                emulator=mode=="emulator";
                break;
            }
            case 'a':
                alpha=atof(optarg);
                break;
            case 'c':
                beta=atof(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                binary_file_provided=true;
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
    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    if(binary_file_provided)
    {
        program_path = replace(program_path, "<rank>", std::to_string(rank));
        program_path = replace(program_path, "<type>", std::to_string(rank));
    }
    else
    {
        if(emulator)
        {
            program_path = ("emulator_" + std::to_string(rank));
            if(rank==0) program_path+="/gesummv_rank0.aocx";
            else program_path+="/gesummv_rank1.aocx";
        }
        else
        {
            if(rank==0) program_path="gesummv_rank0/gesummv_rank0.aocx";
            else program_path="gesummv_rank1/gesummv_rank1.aocx";
        }
    }

    std::cerr << "Program: " << program_path << std::endl;

    char hostname[256];
    gethostname(hostname, 256);
    std::cout << "Rank" << rank<<" executing on host:" <<hostname << " program: "<<program_path<<std::endl;

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
    posix_memalign ((void **)&fpga_res_y, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*sizeof(float));

    generate_float_vector(x,m);
    generate_float_matrix(A,n,m);
    generate_float_matrix(B,n,m);


    hlslib::ocl::Context context(fpga);
    auto program = context.MakeProgram(program_path);
    std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>> buffers;
    SMI_Comm comm=SmiInit_gesummv_rank0(rank, rank_count, ROUTING_DIR, context, program, buffers);


    int tile_size=128;
    const int w=64;
    int one=1;
    int zero=0;
    float fzero=0,fone=1;

    cout << "Executing with tile size " << tile_size<<endl;

    // Create device buffers
    size_t elem_per_module=n*m/2;
    hlslib::ocl::Buffer<float, hlslib::ocl::Access::readWrite> input_x = context.MakeBuffer<float, hlslib::ocl::Access::readWrite>(hlslib::ocl::MemoryBank::bank2, m);
    hlslib::ocl::Buffer<float, hlslib::ocl::Access::readWrite> output_y = context.MakeBuffer<float, hlslib::ocl::Access::readWrite>(hlslib::ocl::MemoryBank::bank3, n);
    hlslib::ocl::Buffer<float, hlslib::ocl::Access::readWrite> input_M_0 = context.MakeBuffer<float, hlslib::ocl::Access::readWrite>(hlslib::ocl::MemoryBank::bank0, elem_per_module);
    hlslib::ocl::Buffer<float, hlslib::ocl::Access::readWrite> input_M_1 = context.MakeBuffer<float, hlslib::ocl::Access::readWrite>(hlslib::ocl::MemoryBank::bank1, elem_per_module);


     // Create kernels
    int x_repetitions=ceil((float)(n)/tile_size);
    std::vector<hlslib::ocl::Kernel> kernels;
    kernels.emplace_back(program.MakeKernel("kernel_read_matrix_A_0", input_M_0, input_M_1,n,m,m));
    kernels.emplace_back(program.MakeKernel("kernel_read_vector_x_0", input_x, m, tile_size, x_repetitions));
    kernels.emplace_back(program.MakeKernel("kernel_read_vector_y_0", output_y, zero, zero, zero));

    hlslib::ocl::Kernel *gemv, *axpy;

    //Copy data
    cout << "Copying data to device..." <<endl;
    if(rank==0){

        //prepare data
        //copy the matrix interleaving it into two modules
        size_t offset=0;
        size_t increment=w/2*sizeof(float); //attention to the way in which w is used
        const int loop_it=((int)(m))/w;   //W must be a divisor of M
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<loop_it;j++)
            {
                //write to the different banks
                kernels[0].commandQueue().enqueueWriteBuffer(input_M_0.devicePtr(), CL_FALSE,offset, (w/2)*sizeof(float),&(A[i*m+j*w]));
                kernels[0].commandQueue().enqueueWriteBuffer(input_M_1.devicePtr(), CL_FALSE,offset, (w/2)*sizeof(float),&(A[i*m+j*w+32]));

                offset+=increment;
            }
        }
        kernels[0].commandQueue().finish();

        input_x.CopyFromHost(0,m,x);

        kernels.emplace_back(program.MakeKernel("gemv", one, n, m, alpha, fzero));
        kernels.emplace_back(program.MakeKernel("axpy", fone, n, comm));
        kernels.emplace_back(program.MakeKernel("kernel_write_vector_0", output_y, n, tile_size));
    }
    else{

        //copy the matrix interleaving it into two modules
        size_t offset=0;
        size_t increment=w/2*sizeof(float);
        const int loop_it=((int)(m))/w;   //W must be a divisor of M
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<loop_it;j++)
            {
                //write to the different banks
                kernels[0].commandQueue().enqueueWriteBuffer(input_M_0.devicePtr(), CL_FALSE,offset, (w/2)*sizeof(float),&(B[i*m+j*w]));
                kernels[0].commandQueue().enqueueWriteBuffer(input_M_1.devicePtr(), CL_FALSE,offset, (w/2)*sizeof(float),&(B[i*m+j*w+32]));
                offset+=increment;
            }
        }
        kernels[0].commandQueue().finish();

        input_x.CopyFromHost(0,m,x);

        kernels.emplace_back(program.MakeKernel("gemv", one, n, m, beta, fzero, comm));

    }

    cout << "Ready to start " <<endl;
    std::vector<double> times;

    timestamp_t startt=current_time_usecs();


    //Program startup
    for(int i=0;i<runs;i++)
    {
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        std::future<std::pair<double, double>>  futures[6];

        //start all the kernels
        for(int i=0;i<kernels.size();i++){
            futures[i]=kernels[i].ExecuteTaskAsync();
        }

        //wait
        for(int i=0;i<kernels.size();i++){
            futures[i].wait();
        }
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        if(rank ==0){
            //save execution time
            double max_time = 0;
            for(int i=0;i<kernels.size();i++){
                std::pair<double, double> timings=futures[i].get();
                if (timings.first > max_time)
                    max_time = timings.first;
            }
            times.push_back(max_time * 1e6);
        }

    }
    //Program ends
    timestamp_t endt=current_time_usecs();

    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    if(rank ==0)
    {
        output_y.CopyToHost(0,n,fpga_res_y);

        //check: we can do this since no-one overwrites the input data
        timestamp_t comp_start=current_time_usecs();
        cblas_sgemv(CblasRowMajor,CblasNoTrans,n,m,beta,B,m,x,1,0,y,1);
        cblas_sgemv(CblasRowMajor,CblasNoTrans,n,m,alpha,A,m,x,1,1,y,1);
        timestamp_t cpu_time=current_time_usecs()-comp_start;

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

       //print times
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
        double conf_interval_99=2.58*stddev/sqrt(runs);

        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;

        //save the info into output file
        ofstream fout("distributed_output.dat");
        fout << "#N = "<<n<<", Runs = "<<runs<<endl;
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
