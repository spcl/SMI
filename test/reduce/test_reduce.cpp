/**
    Gather
    Test must be executed with 8 ranks

    Once built, execute it with:
         env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 ./test_gather.exe "./gather_emulator_<rank>.aocx"
 */

#define TEST_TIMEOUT 30

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
hlslib::ocl::Context *context;


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


bool runAndReturn(hlslib::ocl::Kernel &kernel, hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> &check, int root)
{
    //only rank 0 and the recv rank start the app kernels
    MPI_Barrier(MPI_COMM_WORLD);
    
    kernel.ExecuteTask();

    
    MPI_Barrier(MPI_COMM_WORLD);
    //check
    if(my_rank==root)
    {
        char res;
        check.CopyToHost(&res);
        return res==1;
    }
    else
        return true;
}

TEST(Reduce, MPIinit)
{
    ASSERT_EQ(rank_count,8);
}


TEST(Reduce, CharMax)
{
    //with this test we evaluate the correcteness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_char_max");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {

            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                //printf("root: %d ml: %d, it:%d\n",root, ml,i);
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}


TEST(Reduce, ShortMin)
{
    //with this test we evaluate the correcteness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_short_min");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                //printf("root: %d ml: %d, it:%d\n",root, ml,i);
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}
TEST(Reduce, IntAdd)
{
    //with this test we evaluate the correcteness of integer messages transmission

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_int_add");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                //printf("root: %d ml: %d, it:%d\n",root, ml,i);
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}

TEST(Reduce, IntMax)
{

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_int_max");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                //printf("root: %d ml: %d, it:%d\n",root, ml,i);
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}

TEST(Reduce, FloatAdd)
{
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_float_add");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}

TEST(Reduce, DoubleAdd)
{

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_double_add");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}

TEST(Reduce, IntMaxAD)
{

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_int_max_ad");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}

TEST(Reduce, FloatMinAD)
{

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::readWrite> check = context->MakeBuffer<char, hlslib::ocl::Access::readWrite>(1);
    hlslib::ocl::Kernel kernel = context->CurrentlyLoadedProgram().MakeKernel("test_float_min");

    std::vector<int> message_lengths={1,128, 300};
    std::vector<int> roots={1,4,7};
    int runs=2;
    for(int root:roots)    //consider different roots
    {

        for(int ml:message_lengths)     //consider different message lengths
        {
            cl::Kernel cl_kernel = kernel.kernel();
            cl_kernel.setArg(0,sizeof(int),&ml);
            cl_kernel.setArg(1,sizeof(char),&root);
            cl_kernel.setArg(2,sizeof(cl_mem),&check.devicePtr());
            cl_kernel.setArg(3,sizeof(SMI_Comm),&comm);

            for(int i=0;i<runs;i++)
            {
                if(my_rank==0)  //remove emulated channels
                    system("rm emulated_chan* 2> /dev/null;");

                ASSERT_DURATION_LE(TEST_TIMEOUT, {
                  ASSERT_TRUE(runAndReturn(kernel,check,root));
                });

            }
        }
    }
}



int main(int argc, char *argv[])
{


     //   std::cerr << "Usage: [env CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 " << argv[0] << " \"<fpga binary file>\"" << std::endl;

    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    //delete listeners for all the rank except 0
    if(argc==2)
        program_path =argv[1];
    else
        program_path="emulator_<rank>/reduce.aocx";
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
    comm=SmiInit_reduce(my_rank, rank_count, ROUTING_DIR, *context,  program, buffers);


    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;

}
