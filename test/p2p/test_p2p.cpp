/**
    P2P Test.
    Test must be executed with 8 ranks
 */

#define TEST_TIMEOUT 10

#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <utils/ocl_utils.hpp>
#include <utils/utils.hpp>
#include <limits.h>
#include <cmath>
#include <thread>
#include <future>
#include "smi_generated_host.c"
#define ROUTING_DIR "smi-routes/"
using namespace std;
std::string program_path;
int rank_count, my_rank;

cl::Platform  platform;
cl::Device device;
cl::Context context;
cl::Program program;
std::vector<cl::Buffer> buffers;
SMI_Comm comm;    
//https://github.com/google/googletest/issues/348#issuecomment-492785854
#define ASSERT_DURATION_LE(secs, stmt) { \
  std::promise<bool> completed; \
  auto stmt_future = completed.get_future(); \
  std::thread([&](std::promise<bool>& completed) { \
    stmt; \
    completed.set_value(true); \
  }, std::ref(completed)).detach(); \
  if(stmt_future.wait_for(std::chrono::seconds(secs)) == std::future_status::timeout){ \
    GTEST_FATAL_FAILURE_("       timed out (> " #secs \
    " seconds). Check code for infinite loops"); \
    MPI_Finalize();\
    } \
}


bool runAndReturn(cl::CommandQueue &queue, cl::Kernel &kernel, cl::Buffer &check,int my_rank, int recv_rank)
{
    //only rank 0 and the recv rank start the app kernels
    MPI_Barrier(MPI_COMM_WORLD);
    if(my_rank==0 || my_rank==recv_rank)
    {
        queue.enqueueTask(kernel);

        queue.finish();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(my_rank==recv_rank)
    {
        //check
        char res;
        queue.enqueueReadBuffer(check,CL_TRUE,0,1,&res);
        return res==1;
    }
    else
        return true;
}

TEST(P2P, MPIinit)
{
    ASSERT_EQ(rank_count,8);
}

TEST(P2P, CharMessages)
{

    MPI_Barrier(MPI_COMM_WORLD);
    //float messages

    //create the program
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"test_char",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    std::vector<int> message_lengths={1,128,1024,100000};
    std::vector<int> receivers={1,4,7};
    int runs=2;
    for(int recv_rank:receivers)    //consider different receivers
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            if(my_rank==0)
            {
                char dest=(char)recv_rank;
                kernel.setArg(0,sizeof(int),&ml);
                kernel.setArg(1,sizeof(char),&dest);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }
            else
            {
                kernel.setArg(0,sizeof(cl_mem),&check);
                kernel.setArg(1,sizeof(int),&ml);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(queue,kernel,check,my_rank,recv_rank));
                });
            }
        }
    }
}

TEST(P2P, ShortMessages)
{

    MPI_Barrier(MPI_COMM_WORLD);
    //float messages

    //create the program
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"test_short",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    std::vector<int> message_lengths={1,128,1024,100000};
    std::vector<int> receivers={1,4,7};
    int runs=2;
    for(int recv_rank:receivers)    //consider different receivers
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            if(my_rank==0)
            {
                char dest=(char)recv_rank;
                kernel.setArg(0,sizeof(int),&ml);
                kernel.setArg(1,sizeof(char),&dest);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }
            else
            {
                kernel.setArg(0,sizeof(cl_mem),&check);
                kernel.setArg(1,sizeof(int),&ml);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");
                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(queue,kernel,check,my_rank,recv_rank));
                });
            }
        }
    }
}
TEST(P2P, IntegerMessages)
{
    //with this test we evaluate the correcteness of integer messages transmission
  
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"test_int",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    std::vector<int> message_lengths={1,128,1024,100000};
    std::vector<int> receivers={1,4,7};
    int runs=2;
    for(int recv_rank:receivers)    //consider different receivers
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            if(my_rank==0)
            {
                char dest=(char)recv_rank;
                kernel.setArg(0,sizeof(int),&ml);
                kernel.setArg(1,sizeof(char),&dest);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }
            else
            {
                kernel.setArg(0,sizeof(cl_mem),&check);
                kernel.setArg(1,sizeof(int),&ml);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(queue,kernel,check,my_rank,recv_rank));
                });
            }
        }
    }
}
TEST(P2P, FloatMessages)
{

    MPI_Barrier(MPI_COMM_WORLD);
    //float messages

    //create the program
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"test_float",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    std::vector<int> message_lengths={1,128,1024,100000};
    std::vector<int> receivers={1,4,7};
    int runs=2;
    for(int recv_rank:receivers)    //consider different receivers
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            if(my_rank==0)
            {
                char dest=(char)recv_rank;
                kernel.setArg(0,sizeof(int),&ml);
                kernel.setArg(1,sizeof(char),&dest);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }
            else
            {
                kernel.setArg(0,sizeof(cl_mem),&check);
                kernel.setArg(1,sizeof(int),&ml);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(queue,kernel,check,my_rank,recv_rank));
                });
            }
        }
    }
}

TEST(P2P, DoubleMessages)
{

    MPI_Barrier(MPI_COMM_WORLD);
    //float messages

    //create the program
    cl::Kernel kernel;
    cl::CommandQueue queue;
    IntelFPGAOCLUtils::createCommandQueue(context,device,queue);
    IntelFPGAOCLUtils::createKernel(program,"test_double",kernel);

    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);
    std::vector<int> message_lengths={1,128,1024,100000};
    std::vector<int> receivers={1,4,7};
    int runs=2;
    for(int recv_rank:receivers)    //consider different receivers
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            if(my_rank==0)
            {
                char dest=(char)recv_rank;
                kernel.setArg(0,sizeof(int),&ml);
                kernel.setArg(1,sizeof(char),&dest);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }
            else
            {
                kernel.setArg(0,sizeof(cl_mem),&check);
                kernel.setArg(1,sizeof(int),&ml);
                kernel.setArg(2,sizeof(SMI_Comm),&comm);
            }

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(queue,kernel,check,my_rank,recv_rank));
                });
            }
        }
    }
}

int main(int argc, char *argv[])
{


    if(argc<2)
    {
        std::cerr << "Usage: [env CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 " << argv[0] << " <fpga binary file with <rank> and <type> flags>" << std::endl;
        return -1;
    }

    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    //delete listeners for all the rank except 0
    program_path =argv[1];
    ::testing::TestEventListeners& listeners =
            ::testing::UnitTest::GetInstance()->listeners();
    CHECK_MPI(MPI_Init(&argc, &argv));

    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &rank_count));
    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &my_rank));
    if (my_rank!= 0) {
        delete listeners.Release(listeners.default_result_printer());
    }

    //create environemnt
    int fpga=my_rank%2;
    program_path = replace(program_path, "<rank>", std::to_string(my_rank));
    if(my_rank==0)
        program_path = replace(program_path, "<type>", std::string("0"));
    else
        program_path = replace(program_path, "<type>", std::string("1"));
    comm=SmiInit_p2p_rank0(my_rank, rank_count, program_path.c_str(), ROUTING_DIR, platform, device, context, program, fpga,buffers);


    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;

}
