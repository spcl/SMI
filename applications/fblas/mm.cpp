/**
 * Matrix matrix multiplication test HLS modules
 * Computes C=A*B
 */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "CL/opencl.h"
#include <fstream>
#include <cmath>
#include <cassert>
#include <cblas.h>

#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"
#define ROUTING_DIR "applications/fblas/mm_routing_crossbar/"

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



    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    int fpga = rank % 2; // in this case is ok, pay attention
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cerr << "Program: " << program_path << std::endl;

    //init data: matrices should be memorized as flat arrays (useful for kernel execution)
    float *A,*B,*C,*fpgaC, *globalA,*globalB,*globalC;
    float alpha=1.0f;
    float beta=.0f; //We compute C=A*B

    //each rank will have a portion of N/RxP of A and PxM/R of B
    //and computes NxM/R of C
    assert(n%rank_count==0);
    assert(m%rank_count==0);
    int nrow_partition=n/rank_count;
    int mcol_partition=m/rank_count;

    if(rank==0)
    {

        posix_memalign ((void **)&globalA, align, n*p*sizeof(float));
        posix_memalign ((void **)&globalB, align, p*m*sizeof(float));
        posix_memalign ((void **)&globalC, align, n*m*sizeof(float));
        generate_float_matrix(globalA,n,p);
        generate_float_matrix(globalB,p,m);

    }
#if defined(CHECK)
        posix_memalign ((void **)&fpgaC, IntelFPGAOCLUtils::AOCL_ALIGNMENT, n*mcol_partition*sizeof(float));
#endif
    posix_memalign ((void **)&A, align, nrow_partition*p*sizeof(float));
    posix_memalign ((void **)&B, align, p*mcol_partition*sizeof(float));
    posix_memalign ((void **)&C, align, n*mcol_partition*sizeof(float));

    //scatter A
    int elems_per_rank=nrow_partition*p;
    MPI_Scatter(globalA,elems_per_rank,MPI_FLOAT,A,elems_per_rank,MPI_FLOAT,0,MPI_COMM_WORLD);

    //now check...
   /* for(int i=0;i<nrow_partition;i++)
        for(int j=0;j<p;j++)
            if(A[i*p+j]!=i+rank*nrow_partition)
                printf("Error on rank %d: expected %.0f instead of %.0f\n",rank,i+rank*nrow_partition,A[i*p+j]);
*/
    //scatter B (done row by row...alternatively you can use MPI data types)
    elems_per_rank=mcol_partition;
    for(int i=0;i<p;i++)
        MPI_Scatter(&(globalB[i*m]),elems_per_rank,MPI_FLOAT,&(B[i*mcol_partition]),elems_per_rank,MPI_FLOAT,0,MPI_COMM_WORLD);

    //check...
  /*  for(int i=0;i<p;i++)
        for(int j=0;j<mcol_partition;j++)
            if(B[i*mcol_partition+j]!=i)
                printf("[%d][%d] Error on rank %d: expected %.0f instead of %.0f\n",i,j,rank,(float)i,B[i*mcol_partition+j]);
*/

    cl_int status;
    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"read_matrix_A","read_matrix_B","sgemm","write_matrix", "kernel_bcast",
                                           "CK_S_0","CK_S_1","CK_S_2","CK_S_3","CK_R_0","CK_R_1","CK_R_2","CK_R_3"};
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //------ COMPUTATION KERNELS -----------

    //create memory buffers
    cl::Buffer input_A(context,CL_MEM_READ_ONLY|CL_CHANNEL_1_INTELFPGA,nrow_partition*p*sizeof(float));
    cl::Buffer input_B(context,CL_MEM_READ_ONLY|CL_CHANNEL_2_INTELFPGA,p*mcol_partition*sizeof(float));
    cl::Buffer output_C(context,CL_MEM_READ_WRITE|CL_CHANNEL_3_INTELFPGA,n*mcol_partition*sizeof(float));
    queues[0].enqueueWriteBuffer(input_A,CL_TRUE,0,nrow_partition*p*sizeof(float),A);
    queues[0].enqueueWriteBuffer(input_B,CL_TRUE,0,p*mcol_partition*sizeof(float),B);
    queues[0].enqueueWriteBuffer(output_C,CL_TRUE,0,n*mcol_partition*sizeof(float),C);

    //read_matrix A: pay attention to the argument, a bit strange wrt before
    //since now nrow_partition rows are local, the rest is broadcasted
    //so the read_matrix_A kernel repeat the injection of A for rank_times
    kernels[0].setArg(0,sizeof(cl_mem),&input_A);
    kernels[0].setArg(1,sizeof(int),&nrow_partition);
    kernels[0].setArg(2,sizeof(int),&p);
    kernels[0].setArg(3,sizeof(int),&mcol_partition);
    kernels[0].setArg(4,sizeof(int),&p);
    kernels[0].setArg(5,sizeof(char),&rank);
    kernels[0].setArg(6,sizeof(char),&rank_count);

    //read_matrix B
    kernels[1].setArg(0,sizeof(cl_mem),&input_B);
    kernels[1].setArg(1,sizeof(int),&n);
    kernels[1].setArg(2,sizeof(int),&p);
    kernels[1].setArg(3,sizeof(int),&mcol_partition);
    kernels[1].setArg(4,sizeof(int),&mcol_partition);

    //sgemm
    kernels[2].setArg(0,sizeof(int),&n);
    kernels[2].setArg(1,sizeof(int),&mcol_partition);
    kernels[2].setArg(2,sizeof(int),&p);
    kernels[2].setArg(3,sizeof(float),&alpha);

    //write results
    kernels[3].setArg(0,sizeof(cl_mem),&output_C);
    kernels[3].setArg(1,sizeof(float),&beta);
    kernels[3].setArg(2,sizeof(int),&n);
    kernels[3].setArg(3,sizeof(int),&mcol_partition);
    kernels[3].setArg(4,sizeof(int),&mcol_partition);

    // ------  SMI_KERNEL -------------

    char tags=1;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);

    //load ck_r
    char routing_tables_ckr[4]; //only one tag
    char routing_tables_cks[4][rank_count]; //4 ranks
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, 1, ROUTING_DIR, "ckr", &routing_tables_ckr[i]);
        LoadRoutingTable<char>(rank, i, rank_count, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
    }
    //std::cout << "Rank: "<< rank<<endl;
    // for(int i=0;i<kChannelsPerRank;i++)
    //    for(int j=0;j<rank_count;j++)
    //        std::cout << i<< "," << j<<": "<< (int)routing_tables_cks[i][j]<<endl;

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,&routing_tables_ckr[0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,&routing_tables_ckr[1]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,&routing_tables_ckr[2]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,&routing_tables_ckr[3]);

    //bcast kernel
    kernels[4].setArg(0,sizeof(char),&rank_count);


    //args for the CK_Ss
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[8].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);

    //args for the CK_Rs
    kernels[9].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[9].setArg(1,sizeof(char),&rank);
    kernels[10].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[10].setArg(1,sizeof(char),&rank);
    kernels[11].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[11].setArg(1,sizeof(char),&rank);
    kernels[12].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[12].setArg(1,sizeof(char),&rank);

    //start the CKs
    const int num_kernels=kernel_names.size();

    for(int i=num_kernels-1;i>=num_kernels-9;i--) //start also the bcast kernel
        queues[i].enqueueTask(kernels[i]);


    std::vector<double> times;

    for(int r=0;r<runs;r++)
    {
        cl::Event events[4];
        for(int i=0;i<4;i++)
            queues[i].enqueueTask(kernels[i],nullptr,&events[i]);

        //wait
        for(int i=0;i<4;i++)
            queues[i].finish();

        ulong min_start=4294967295, max_end=0;
        ulong end;
        ulong start;
        for(int i=0;i<4;i++)
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
    //fcopy back to host the C partition computed in this FPGA
#if defined(CHECK)
    queues[0].enqueueReadBuffer(output_C,CL_TRUE,0,n*mcol_partition*sizeof(float),fpgaC);
    usleep(100000*rank);
    printf("RANK: %d\n",rank);
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<mcol_partition;j++)
            printf("%.0f ",fpgaC[i*mcol_partition+j]);
        printf("\n");
    }

    //gather it

    for(int i=0;i<n;i++)
        MPI_Gather(&(fpgaC[i*mcol_partition]),mcol_partition,MPI_FLOAT,&(globalC[i*m]),mcol_partition,MPI_FLOAT,0,MPI_COMM_WORLD);
    //check
    if(rank==0)
    {
        float *blasC;
        posix_memalign ((void **)&blasC, align, n*m*sizeof(float));
        cblas_sgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,n,m,p,alpha,globalA,p,globalB,m,beta,blasC,m);
        const double flteps = 1e-4;
        bool ok=true;

        for(int i=0;i<n;i++)
        {
            for(int j=0;j<m;j++)
            {
                if(!test_equals(globalC[i*m+j],blasC[i*m+j],flteps))
                {
                    std::cout <<"["<<i<<", "<<j<<" ] "<<globalC[i*m+j]<<"\t"<<blasC[i*m+j]<<std::endl;
                    ok=false;
                }
            }
        }

        /*  for(int i=0;i<m;i++)
         {
             for(int r=0;r<rank_count;r++)
                 for(int j=0;j<mcol_partition;j++)
                 {
                     if(globalC[i*m+r*mcol_partition+j]!=r)
                         printf("[%d][%d] Error on rank %d: expected %.0f instead of %.0f\n",i,r*mcol_partition+j,rank,(float)r,globalC[i*m+r*mcol_partition+j]);

                 }
         }*/


    if(ok) std::cout << "Computation Ok!" <<std::endl;
    else std::cout << "ERRORR!!!!!!!" << std::endl;
    }

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
