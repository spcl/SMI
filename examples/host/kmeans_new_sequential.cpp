

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../include/kmeans_new.h"
#include <random>
#include "hlslib/intel/OpenCL.h"


using Data_t = DTYPE;
constexpr int kNumTags = 6;
constexpr int kK = K;
constexpr int kDims = DIMS;
using AlignedVec_t =
std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 64>>;
using Point_t = std::array<Data_t, kDims>;

using namespace std;
int main(int argc, char *argv[])
{


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
    int fpga=0,iterations;
    int rank,runs=1;
    while ((c = getopt (argc, argv, "n:b:i:r:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'i':
                iterations=atoi(optarg);
                break;
            case 'r':
                runs=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"ComputeMeans", "SendCentroids", "ComputeDistance"};

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);


    int rank_count=1;
    //Application
    int points_per_rank = n / rank_count;
    int mpi_rank=rank;
    int mpi_size=rank_count;
    const int num_points = n;

    AlignedVec_t input;
    AlignedVec_t points(kDims * points_per_rank);
    AlignedVec_t centroids(kK * kDims);
    AlignedVec_t centroids_host(kK * kDims);    //for host checking
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
        /*for (int k = 0; k < kK; ++k) {
            const int n_per_centroid = num_points / kK;
            for (int i = k * n_per_centroid; i < (k + 1) * n_per_centroid; ++i) {
                printf("Generating point %d\n",i);
                for (int d = 0; d < kDims; ++d) {
                    input[i * kDims + d] =
                            normal_dist(rng) + gaussian_means[k * kDims + d];
                }
            }
        }*/
        //BUG TO FIX, if num_points is not divisible by kK
        for(int i=0;i<num_points;i++)
        {
            int k=i%kK;
            for (int d = 0; d < kDims; ++d) {
                input[i * kDims + d] =
                        normal_dist(rng) + gaussian_means[k * kDims + d];
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
        std::stringstream ss;

        /*ss.str("");
        ss.clear();
        ss << "Means used to generate data:\n";
        ss << "Initial centroids:\n";
        for (int k = 0; k < kK; ++k) {
            ss << "  {" << centroids[k * kDims];
            for (int d = 1; d < kDims; ++d) {
                ss << ", " << centroids[k * kDims + d];
            }
            ss << "}\n";
        }
        std::cout <<ss.str()<<std::endl;*/
        std::copy(centroids.begin(),centroids.end(),centroids_host.begin());
    }


    // Distribute data

    cl::Buffer points_device(context,CL_MEM_READ_WRITE,sizeof(float)*(input.size()));
    cl::Buffer centroids_device_read(context,CL_MEM_READ_ONLY,sizeof(float)*(centroids.size()));
    cl::Buffer centroids_device_write(context,CL_MEM_WRITE_ONLY,sizeof(float)*(centroids.size()));





    kernels[0].setArg(0,sizeof(cl_mem),&centroids_device_write);
    kernels[0].setArg(1,sizeof(int),&points_per_rank);
    kernels[0].setArg(2,sizeof(int),&iterations);
    kernels[0].setArg(3,sizeof(int),&mpi_rank);
    kernels[0].setArg(4,sizeof(int),&mpi_size);

    //sen centroids
    kernels[1].setArg(0,sizeof(cl_mem),&centroids_device_read);
    kernels[1].setArg(1,sizeof(int),&iterations);
    kernels[1].setArg(2,sizeof(int),&mpi_rank);
    kernels[1].setArg(3,sizeof(int),&mpi_size);

    kernels[2].setArg(0,sizeof(cl_mem),&points_device);
    kernels[2].setArg(1,sizeof(int),&points_per_rank);
    kernels[2].setArg(2,sizeof(int),&iterations);
    kernels[2].setArg(3,sizeof(int),&mpi_rank);
    kernels[2].setArg(4,sizeof(int),&mpi_size);

    std::vector<double> times;

    cl::Event events[3];
    // wait for other nodes
    for(int i=0;i<runs;i++)
    {
        queues[0].enqueueWriteBuffer(points_device, CL_TRUE,0,sizeof(float)*(input.size()),input.data());
        queues[0].enqueueWriteBuffer(centroids_device_read, CL_TRUE,0,sizeof(float)*(centroids.size()),centroids.data());
        queues[0].enqueueWriteBuffer(centroids_device_write, CL_TRUE,0,sizeof(float)*(centroids.size()),centroids.data());


        queues[0].enqueueTask(kernels[0],nullptr,&events[0]);
        queues[1].enqueueTask(kernels[1],nullptr,&events[1]);
        queues[2].enqueueTask(kernels[2],nullptr,&events[2]);

        queues[0].finish();
        queues[1].finish();
        queues[2].finish();



        if(rank==0)
        {
            ulong min_start=4294967295, max_end=0;
            ulong end, start;
            for(int i=0;i<3;i++)
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
            double time= (double)((max_end-min_start)/1000.0f);
            times.push_back(time);
        }
    }

    //copy centroids back
    queues[0].enqueueReadBuffer(points_device, CL_TRUE,0,sizeof(float)*(input.size()),input.data());


    //get back the results
    queues[0].enqueueReadBuffer(centroids_device_write,CL_TRUE,0,sizeof(float)*(centroids.size()),centroids.data());
    // Final centroids
    std::cout << "Final centroids:\n";
    for (int k = 0; k < kK; ++k) {
        std::cout << "  {" << centroids[k * kDims];
        for (int d = 1; d < kDims; ++d) {
            std::cout << ", " << centroids[k * kDims + d];
        }
        std::cout << "}\n";
    }
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
    std::ostringstream filename;
    filename << "smi_kmeans_"<<rank_count <<"_"<< n << "_"<<iterations<<"it.dat";
    ofstream fout(filename.str());
    fout << "#N = "<<n<<", Iterations = "<<iterations<< ", Runs = "<<runs<<endl;
    fout << "#Ranks = " <<rank_count<<endl;
    fout << "#Average Computation time (usecs): "<<mean<<endl;
    fout << "#Standard deviation (usecs): "<<stddev<<endl;
    fout << "#Confidence interval 99%: +- "<<conf_interval_99<<endl;
    fout << "#Execution times (usecs):"<<endl;
    for(auto t:times)
        fout << t << endl;
    fout.close();





}
