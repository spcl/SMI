/**
    Broadcast
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
#include <hlslib/intel/OpenCL.h>
#include "smi_generated_host.c"
#define ROUTING_DIR "smi-routes/"
using namespace std;
std::string program_path;
int rank_count, my_rank;
hlslib::ocl::Context *context; //global context
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


bool runAndReturn(hlslib::ocl::Kernel &kernel, hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> &check)
{
    //only rank 0 and the recv rank start the app kernels
    MPI_Barrier(MPI_COMM_WORLD);

    kernel.ExecuteTask();
    
    MPI_Barrier(MPI_COMM_WORLD);
    //check
    char res;
    check.CopyToHost(&res);
    return res==1;
}

TEST(Broadcast, MPIinit)
{
    ASSERT_EQ(rank_count,8);
}

TEST(Broadcast, CharMessages)
{
    //with this test we evaluate the correctness of char messages transmission
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_char");
    std::vector<int> message_lengths={1,128,1024,1000};
    std::vector<int> roots={0,4,5};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}


TEST(Broadcast, ShortMessages)
{
    //with this test we evaluate the correctness of integer messages transmission
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_short");

    std::vector<int> message_lengths={1,128,1024,10000};
    std::vector<int> roots={0,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds x seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}

TEST(Broadcast, IntegerMessages)
{
    //with this test we evaluate the correctness of integer messages transmission
  
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_int");

    std::vector<int> message_lengths={1,128,1024,10000};
    std::vector<int> roots={0,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}

TEST(Broadcast, FloatMessages)
{
    //with this test we evaluate the correctness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_float");

    std::vector<int> message_lengths={1,128,1024,10000};
    std::vector<int> roots={0,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}

TEST(Broadcast, DoubleMessages)
{
    //with this test we evaluate the correctness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_double");

    std::vector<int> message_lengths={1,128,1024,10000};
    std::vector<int> roots={0,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}


TEST(Broadcast, IntegerMessagesAD)
{
    //with this test we evaluate the correcteness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_int_ad");

    std::vector<int> message_lengths={1,128,1024,10000};
    std::vector<int> roots={0,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}

TEST(Broadcast, FloatMessagesAD)
{
    //with this test we evaluate the correctness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_float_ad");

    std::vector<int> message_lengths={1,128,1024,10000};
    std::vector<int> roots={0,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(1,sizeof(int),&ml);
            cl_kernel.setArg(2,sizeof(char),&root);
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");


                // run some_function() and compared with some_value
                // but end the function if it exceeds 3 seconds
                //source https://github.com/google/googletest/issues/348#issuecomment-492785854
                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check));
                });
            }
        }
    }
}

int main(int argc, char *argv[])
{

    //std::cerr << "Usage: [env CL_CONFIG_CPU_EMULATE_DEVICES=8 mpirun -np 8 " << argv[0] << " [<fpga binary file>]" << std::endl;

    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    //delete listeners for all the rank except 0
    if(argc==2)
        program_path =argv[1];
    else
        program_path="emulator_<rank>/broadcast.aocx";
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
    context = new hlslib::ocl::Context();
    auto program =  context->MakeProgram(program_path);
    std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>> buffers;
    comm=SmiInit_broadcast(my_rank, rank_count, ROUTING_DIR, *context, program, buffers);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;

}
