

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"
#include "../include/kmeans_new.h"
#include <random>
#include "hlslib/intel/OpenCL.h"


#define ROUTING_DIR "examples/host/k_means_routing/"
using Data_t = DTYPE;
constexpr int kNumTags = 6;
constexpr int kK = K;
constexpr int kDims = DIMS;
using AlignedVec_t =
std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 64>>;

using namespace std;
int main(int argc, char *argv[])
{

    CHECK_MPI(MPI_Init(&argc, &argv));

    //command line argument parsing
    if(argc<7)
    {
        cerr << "Reduce benchmark " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -i <number of iterations> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char root;
    int fpga,iterations;
    int rank;
    while ((c = getopt (argc, argv, "n:b:i:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'i':
                iterations=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'r':
                root=(char)atoi(optarg);
                break;

            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    int rank_count;
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    fpga = rank % 2; // in this case is ok, pay attention
    //fpga=0; //executed on 15 and 16
    std::cout << "Rank: " << rank << " out of " << rank_count << " ranks" << std::endl;
    program_path = replace(program_path, "<rank>", std::to_string(rank));
    std::cout << "Program: " << program_path << " executed on fpga: "<<fpga<<std::endl;
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    printf("Rank %d executing on host: %s\n",rank,hostname);
    float power;
    size_t returnedSize;


    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"CK_S_0", "CK_S_1", "CK_S_2", "CK_S_3", "CK_R_0", "CK_R_1", "CK_R_2", "CK_R_3",
                                          "kernel_reduce_float", "kernel_bcast_float", "kernel_reduce_int","kernel_bcast_int",
                                           "ComputeMeans", "SendCentroids", "ComputeDistance"};

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char tags=6;
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);

    //load ck_r
    char routing_tables_ckr[4][tags]; //two tags
    char routing_tables_cks[4][rank_count]; //4 ranks
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, tags, ROUTING_DIR, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, ROUTING_DIR, "cks", &routing_tables_cks[i][0]);
    }
//    std::cout << "Rank: "<< rank<<endl;
//    for(int i=0;i<kChannelsPerRank;i++)
//        for(int j=0;j<rank_count;j++)
//            std::cout << i<< "," << j<<": "<< (int)routing_tables_cks[i][j]<<endl;
//    sleep(rank);

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags,&routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags,&routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags,&routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags,&routing_tables_ckr[3][0]);


    //args for the CK_Ss
    kernels[0].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);

    //args for the CK_Rs
    kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[4].setArg(1,sizeof(char),&rank);
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[5].setArg(1,sizeof(char),&rank);
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[6].setArg(1,sizeof(char),&rank);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[7].setArg(1,sizeof(char),&rank);

    //start the CKs
    for(int i=0;i<8;i++) //start also the bcast kernel
        queues[i].enqueueTask(kernels[i]);

    //start support kernel for collectives
    kernels[8].setArg(0,sizeof(char),&rank_count);
    kernels[9].setArg(0,sizeof(char),&rank_count);

    kernels[10].setArg(0,sizeof(char),&rank_count);
    kernels[11].setArg(0,sizeof(char),&rank_count);

    queues[8].enqueueTask(kernels[8]);
    queues[9].enqueueTask(kernels[9]);
    queues[10].enqueueTask(kernels[10]);
    queues[11].enqueueTask(kernels[11]);


    //Application
    int points_per_rank = n / rank_count;
    int mpi_rank=rank;
    int mpi_size=rank_count;
    const int num_points = n;

    AlignedVec_t input;
    AlignedVec_t points(kDims * points_per_rank);
    AlignedVec_t centroids(kK * kDims);
    if (mpi_rank == 0) {
        // std::random_device rd;
        std::default_random_engine rng(5);
        input = AlignedVec_t(num_points * kDims);  // TODO: load some data set
        // Generate Gaussian means with a uniform distribution
        std::uniform_real_distribution<Data_t> dist_means(-5, 5);
        AlignedVec_t gaussian_means(kK * kDims);
        // Randomize centers for Gaussian distributions
        for (int k = 0; k < kK; ++k) {
          for (int d = 0; d < kDims; ++d) {
            gaussian_means[k * kDims + d] = dist_means(rng);
          }
        }

        // Generate data with a separate Gaussian at each mean
        std::normal_distribution<Data_t> normal_dist;
        for (int k = 0; k < kK; ++k) {
          const int n_per_centroid = num_points / kK;
          for (int i = k * n_per_centroid; i < (k + 1) * n_per_centroid; ++i) {
            for (int d = 0; d < kDims; ++d) {
              input[i * kDims + d] =
                  normal_dist(rng) + gaussian_means[k * kDims + d];
            }
          }
        }
        // For sampling indices for starting centroids
        std::uniform_int_distribution<size_t> index_dist(0, num_points);
        // Choose initial centroids
        for (int k = 0; k < kK; ++k) {
          auto i = index_dist(rng);
          std::copy(&input[i * kDims], &input[(i + 1) * kDims],
                    &centroids[k * kDims]);
        }
        // Print starting centroids
    }
    // Distribute data
    MPI_Bcast(centroids.data(), kK * kDims, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Scatter(input.data(), points_per_rank, MPI_FLOAT, points.data(),
              points_per_rank, MPI_FLOAT, 0, MPI_COMM_WORLD);


    cl::Buffer points_device(context,CL_MEM_READ_WRITE,sizeof(float)*(points.size()));
    cl::Buffer centroids_device_read(context,CL_MEM_READ_ONLY,sizeof(float)*(centroids.size()));
    cl::Buffer centroids_device_write(context,CL_MEM_WRITE_ONLY,sizeof(float)*(centroids.size()));


    queues[12].enqueueWriteBuffer(points_device, CL_TRUE,0,sizeof(float)*(points.size()),points.data());
    queues[12].enqueueWriteBuffer(centroids_device_read, CL_TRUE,0,sizeof(float)*(centroids.size()),centroids.data());



    kernels[12].setArg(0,sizeof(cl_mem),&centroids_device_write);
    kernels[12].setArg(1,sizeof(int),&points_per_rank);
    kernels[12].setArg(2,sizeof(int),&iterations);
    kernels[12].setArg(3,sizeof(int),&mpi_rank);
    kernels[12].setArg(4,sizeof(int),&mpi_size);

    //sen centroids
    kernels[13].setArg(0,sizeof(cl_mem),&centroids_device_read);
    kernels[13].setArg(1,sizeof(int),&iterations);
    kernels[13].setArg(2,sizeof(int),&mpi_rank);
    kernels[13].setArg(3,sizeof(int),&mpi_size);

    kernels[14].setArg(0,sizeof(cl_mem),&points_device);
    kernels[14].setArg(1,sizeof(int),&points_per_rank);
    kernels[14].setArg(2,sizeof(int),&iterations);
    kernels[14].setArg(3,sizeof(int),&mpi_rank);
    kernels[14].setArg(4,sizeof(int),&mpi_size);

    std::vector<double> times;

  //  kernels[0].setArg(5,sizeof(int),&i);
    cl::Event events[1]; //this defination must stay here
    // wait for other nodes
    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));

    queues[12].enqueueTask(kernels[12],nullptr,&events[0]);
    queues[13].enqueueTask(kernels[13],nullptr,&events[0]);
    queues[14].enqueueTask(kernels[14],nullptr,&events[0]);

    queues[12].finish();
    queues[13].finish();
    queues[14].finish();


    CHECK_MPI(MPI_Barrier(MPI_COMM_WORLD));
    if(rank==root)
    {
        //copy centroids back
        queues[12].enqueueReadBuffer(points_device, CL_TRUE,0,sizeof(float)*(points.size()),points.data());
        ulong end, start;
        events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_START,&start);
        events[0].getProfilingInfo<ulong>(CL_PROFILING_COMMAND_END,&end);
        double time= (double)((end-start)/1000.0f);
        times.push_back(time);
        char res;
        queues[12].enqueueReadBuffer(centroids_device_write,CL_TRUE,0,sizeof(float)*(centroids.size()),centroids.data());
       // Final centroids
        std::cout << "Final centroids:\n";
        for (int k = 0; k < kK; ++k) {
          std::cout << "  {" << centroids[k * kDims];
          for (int d = 1; d < kDims; ++d) {
            std::cout << ", " << centroids[k * kDims + d];
          }
          std::cout << "}\n";
        }


       //check
        double mean=0;
        for(auto t:times)
            mean+=t;
        mean/=iterations;
        //report the mean in usecs
        double stddev=0;
        for(auto t:times)
            stddev+=((t-mean)*(t-mean));
        stddev=sqrt(stddev/iterations);
        double conf_interval_99=2.58*stddev/sqrt(iterations);
        double data_sent_KB=(double)(n*sizeof(float))/1024.0;
        cout << "Computation time (usec): " << mean << " (sttdev: " << stddev<<")"<<endl;
        cout << "Conf interval 99: "<<conf_interval_99<<endl;
        cout << "Conf interval 99 within " <<(conf_interval_99/mean)*100<<"% from mean" <<endl;
        cout << "Sent (KB): " <<data_sent_KB<<endl;
        cout << "Average bandwidth (Gbit/s): " <<  (data_sent_KB*8/(mean/1000000.0))/(1024*1024) << endl;

    }

    CHECK_MPI(MPI_Finalize());

}
