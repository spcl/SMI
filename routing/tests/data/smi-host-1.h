#define __HOST_PROGRAM__
#include <utils/smi_utils.hpp>
#include <vector>
#include <smi/communicator.h>

SMI_Comm SmiInit_program(
        int rank,
        int ranks_count,
        const char* program_path,
        const char* routing_dir,
        cl::Platform &platform,
        cl::Device &device,
        cl::Context &context,
        cl::Program &program,
        int fpga,
        std::vector<cl::Buffer> &buffers)
{
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    // channel kernels
    kernel_names.push_back("smi_kernel_cks_0");
    kernel_names.push_back("smi_kernel_ckr_0");
    kernel_names.push_back("smi_kernel_cks_1");
    kernel_names.push_back("smi_kernel_ckr_1");
    kernel_names.push_back("smi_kernel_cks_2");
    kernel_names.push_back("smi_kernel_ckr_2");
    kernel_names.push_back("smi_kernel_cks_3");
    kernel_names.push_back("smi_kernel_ckr_3");

        // broadcast kernels
    kernel_names.push_back("smi_kernel_bcast_3");
    kernel_names.push_back("smi_kernel_bcast_4");

        // reduce kernels
    kernel_names.push_back("smi_kernel_reduce_6");

        // scatter kernels

        // gather kernels


    IntelFPGAOCLUtils::initEnvironment(
            platform, device, fpga, context,
            program, program_path, kernel_names, kernels, queues
    );

    // create buffers for CKS/CKR
    const int ports = 7;
    const int cks_table_size = ranks_count;
    const int ckr_table_size = ports * 2;
    cl::Buffer routing_table_ck_s_0(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_0(context, CL_MEM_READ_ONLY, ckr_table_size);
    cl::Buffer routing_table_ck_s_1(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_1(context, CL_MEM_READ_ONLY, ckr_table_size);
    cl::Buffer routing_table_ck_s_2(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_2(context, CL_MEM_READ_ONLY, ckr_table_size);
    cl::Buffer routing_table_ck_s_3(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_3(context, CL_MEM_READ_ONLY, ckr_table_size);

    // load routing tables
    char routing_tables_cks[4][cks_table_size];
    char routing_tables_ckr[4][ckr_table_size];
    for (int i = 0; i < 4; i++)
    {
        LoadRoutingTable<char>(rank, i, cks_table_size, routing_dir, "cks", &routing_tables_cks[i][0]);
        LoadRoutingTable<char>(rank, i, ckr_table_size, routing_dir, "ckr", &routing_tables_ckr[i][0]);
    }

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE, 0, cks_table_size, &routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE, 0, cks_table_size, &routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE, 0, cks_table_size, &routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE, 0, cks_table_size, &routing_tables_cks[3][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[3][0]);

    char char_ranks_count=ranks_count;
    char char_rank=rank;
    // cks_0
    kernels[0].setArg(0, sizeof(cl_mem), &routing_table_ck_s_0);
    kernels[0].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_0
    kernels[1].setArg(0, sizeof(cl_mem), &routing_table_ck_r_0);
    kernels[1].setArg(1, sizeof(char), &char_rank);
    // cks_1
    kernels[2].setArg(0, sizeof(cl_mem), &routing_table_ck_s_1);
    kernels[2].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_1
    kernels[3].setArg(0, sizeof(cl_mem), &routing_table_ck_r_1);
    kernels[3].setArg(1, sizeof(char), &char_rank);
    // cks_2
    kernels[4].setArg(0, sizeof(cl_mem), &routing_table_ck_s_2);
    kernels[4].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_2
    kernels[5].setArg(0, sizeof(cl_mem), &routing_table_ck_r_2);
    kernels[5].setArg(1, sizeof(char), &char_rank);
    // cks_3
    kernels[6].setArg(0, sizeof(cl_mem), &routing_table_ck_s_3);
    kernels[6].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_3
    kernels[7].setArg(0, sizeof(cl_mem), &routing_table_ck_r_3);
    kernels[7].setArg(1, sizeof(char), &char_rank);
        // broadcast 3
    kernels[8].setArg(0, sizeof(char), &char_ranks_count);
    // broadcast 4
    kernels[9].setArg(0, sizeof(char), &char_ranks_count);

        // reduce 6
    kernels[10].setArg(0, sizeof(char), &char_ranks_count);

    
    

    // move buffers
    buffers.push_back(std::move( routing_table_ck_s_0));
    buffers.push_back(std::move( routing_table_ck_r_0));
    buffers.push_back(std::move( routing_table_ck_s_1));
    buffers.push_back(std::move( routing_table_ck_r_1));
    buffers.push_back(std::move( routing_table_ck_s_2));
    buffers.push_back(std::move( routing_table_ck_r_2));
    buffers.push_back(std::move( routing_table_ck_s_3));
    buffers.push_back(std::move( routing_table_ck_r_3));

    // start the kernels
    const int num_kernels = kernel_names.size();
    for (int i = num_kernels - 1; i >= 0; i--)
    {
        queues[i].enqueueTask(kernels[i]);
        queues[i].flush();
    }

    // return the communicator
    SMI_Comm comm{ char_rank, char_ranks_count };
    return comm;

}
