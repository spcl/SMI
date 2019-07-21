/**
 *  ATTENTION: helpers modified to have better performance (volatile, N %64 ==0 ...)
            THIS WORKS ONLY IN THE CONSIDERED SETUP OVER FPGA-14
            (or in general, in the case in which two FPGA are directly connected each other using channel 0)
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
    CHECK_MPI(MPI_Init(&argc, &argv));
    //command line argument parsing
    if(argc<17)
    {
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <n>  -m <m> -a <alpha> -c <beta> -r <num runs> -k <rank 0/1> -f <fpga>"<<endl;
        exit(-1);
    }

    int c;
    int n,m,runs,rank;
    float alpha,beta;
    std::string program_path,program_path_streamed;;
    std::string json_path;
    int fpga;

    while ((c = getopt (argc, argv, "n:b:r:a:c:k:f:m:")) != -1)
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
    program_path = replace(program_path, "<rank>", std::to_string(rank));
        program_path = replace(program_path, "<type>", std::to_string(rank));

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

    std::vector<double> streamed_times,transfer_times;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Buffer> buffers;
    SMI_Comm comm=SmiInit_gesummv_rank0(rank, rank_count, program_path.c_str(), ROUTING_DIR, platform, device, context, program, fpga,buffers);

    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;
    if(rank==0)
        kernel_names={"gemv", "axpy", "kernel_read_matrix_A_0", "kernel_read_vector_x_0",
                "kernel_read_vector_y_0", "kernel_write_vector_0"};
    else
        kernel_names={"gemv", "kernel_read_matrix_A_0", "kernel_read_vector_x_0",
                "kernel_read_vector_y_0"};      
    const int num_kernels=kernel_names.size();      
    IntelFPGAOCLUtils::createCommandQueues(context,device,queues, num_kernels);
    IntelFPGAOCLUtils::createKernels(program,kernel_names,kernels);

    int tile_size=128;
    cout << "Executing with tile size " << tile_size<<endl;


    //Copy data
    cl::Buffer input_x(context, CL_MEM_READ_ONLY|CL_CHANNEL_3_INTELFPGA, m *sizeof(float));
    cl::Buffer output_y(context, CL_MEM_READ_WRITE|CL_CHANNEL_4_INTELFPGA, n * sizeof(float));
    size_t elem_per_module=n*m/2;
    cl::Buffer input_M_0(context, CL_MEM_READ_ONLY|CL_CHANNEL_1_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer input_M_1(context, CL_MEM_READ_ONLY|CL_CHANNEL_2_INTELFPGA, elem_per_module*sizeof(float));
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,2);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,1);

    cout << "Copying data to device..." <<endl;
    if(rank==0)
    {
        
        //prepare data
        //copy the matrix interleaving it into two modules
        size_t offset=0;
        size_t increment=64/2*sizeof(float);
        const int loop_it=((int)(m))/64;   //W must be a divisor of M
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<loop_it;j++)
            {
                //write to the different banks
                queues[0].enqueueWriteBuffer(input_M_0, CL_FALSE,offset, (64/2)*sizeof(float),&(A[i*m+j*64]));
                queues[0].enqueueWriteBuffer(input_M_1, CL_FALSE,offset, (64/2)*sizeof(float),&(A[i*m+j*64+32]));

                offset+=increment;
            }
        }
        queues[0].finish();
        queues[0].enqueueWriteBuffer(input_x,CL_TRUE,0,m*sizeof(float),x);
        queues[0].finish();

        cout << "Rank 0 data copied" <<std::endl;
        //gemv_A
        int one=1;
        int zero=0;
        float fzero=0,fone=1;
        int x_repetitions=ceil((float)(n)/tile_size);

        //gemv
        kernels[0].setArg(0, sizeof(int),&one);
        kernels[0].setArg(1, sizeof(int),&n);
        kernels[0].setArg(2, sizeof(int),&m);
        kernels[0].setArg(3, sizeof(float),&alpha);
        kernels[0].setArg(4, sizeof(float),&fzero);

        //axpy
        kernels[1].setArg(0, sizeof(float),&fone);
        kernels[1].setArg(1, sizeof(int),&n);
        kernels[1].setArg(2, sizeof(SMI_Comm),&comm);

        //read_matrix_A
        kernels[2].setArg(0, sizeof(cl_mem),&input_M_0);
        kernels[2].setArg(1, sizeof(cl_mem),&input_M_1);
        kernels[2].setArg(2, sizeof(int),&n);
        kernels[2].setArg(3, sizeof(int),&m);
        kernels[2].setArg(4, sizeof(int),&m);


        //read_vector_x
        kernels[3].setArg(0, sizeof(cl_mem),&input_x);
        kernels[3].setArg(1, sizeof(int),&m);
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

    }
    else
    {
        //copy the matrix interleaving it into two modules
        size_t offset=0;
        size_t increment=64/2*sizeof(float);
        const int loop_it=((int)(m))/64;   //W must be a divisor of M
        for(int i=0;i<n;i++)
        {
            for(int j=0;j<loop_it;j++)
            {
                //write to the different banks
                queues[0].enqueueWriteBuffer(input_M_0, CL_FALSE,offset, (64/2)*sizeof(float),&(B[i*m+j*64]));
                queues[0].enqueueWriteBuffer(input_M_1, CL_FALSE,offset, (64/2)*sizeof(float),&(B[i*m+j*64+32]));

                offset+=increment;
            }
        }
        queues[0].finish();
        queues[0].enqueueWriteBuffer(input_x,CL_TRUE,0,m*sizeof(float),x);
        queues[0].finish();

        cout << "Rank 1 data copied"<<endl;
        //gemv_A
        int one=1;
        int zero=0;
        float fzero=0;
        int x_repetitions=ceil((float)(n)/tile_size);

        kernels[0].setArg(0, sizeof(int),&one);
        kernels[0].setArg(1, sizeof(int),&n);
        kernels[0].setArg(2, sizeof(int),&m);
        kernels[0].setArg(3, sizeof(float),&beta);
        kernels[0].setArg(4, sizeof(float),&fzero);
        kernels[0].setArg(5, sizeof(SMI_Comm),&comm);


        //read_matrix_B
        kernels[1].setArg(0, sizeof(cl_mem),&input_M_0);
        kernels[1].setArg(1, sizeof(cl_mem),&input_M_1);
        kernels[1].setArg(2, sizeof(int),&n);
        kernels[1].setArg(3, sizeof(int),&m);
        kernels[1].setArg(4, sizeof(int),&m);


        //read_vector_x
        kernels[2].setArg(0, sizeof(cl_mem),&input_x);
        kernels[2].setArg(1, sizeof(int),&m);
        kernels[2].setArg(2, sizeof(int),&tile_size);
        kernels[2].setArg(3, sizeof(int),&x_repetitions);

        //read_vector_y useless
        kernels[3].setArg(0, sizeof(cl_mem),&output_y);
        kernels[3].setArg(1, sizeof(int),&zero);
        kernels[3].setArg(2, sizeof(int),&zero);
        kernels[3].setArg(3, sizeof(int),&zero);

    }

    cout << "Ready to start " <<endl;
    std::vector<double> times;

    timestamp_t startt=current_time_usecs();


    //Program startup
    for(int i=0;i<runs;i++)
    {
        cl::Event events[6]; //this defination must stay here
        // wait for other nodes
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        //ATTENTION: If you are executing on the same host
        //since the PCIe is shared that could be problems in taking times
        //This mini sleep should resolve
        if(rank==0)
            usleep(10000);
        //Start computation
        for(int i=0;i<num_kernels;i++)
            queues[i].enqueueTask(kernels[i],nullptr,&events[i]);
        for(int i=0;i<kernel_names.size();i++)
            queues[i].finish();

        //wait
        CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

        //take times
        //compute execution time using OpenCL profiling
        ulong min_start=4294967295, max_end=0;
        ulong end;
        ulong start;
        for(int i=0;i<num_kernels;i++)
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
    //Program ends
    timestamp_t endt=current_time_usecs();
   
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    
    if(rank ==0)
    {
         queues[0].enqueueReadBuffer(output_y,CL_FALSE,0,n*sizeof(float),fpga_res_y);
        //check: we can do this since no-one overwrites the input data
        timestamp_t comp_start=current_time_usecs();
        cblas_sgemv(CblasRowMajor,CblasNoTrans,n,m,beta,B,m,x,1,0,y,1);
        cblas_sgemv(CblasRowMajor,CblasNoTrans,n,m,alpha,A,m,x,1,1,y,1);
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
