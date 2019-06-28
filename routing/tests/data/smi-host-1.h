#include <utils/smi_utils.hpp>

void SmiInit(
        char rank,
        char rank_count,
        const char* program_path,
        const char* routing_dir,
        cl::Platform &platform, 
        cl::Device &device, 
        cl::Context &context, 
        cl::Program &program, 
        int fpga)
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

    IntelFPGAOCLUtils::initEnvironment(
            platform, device, fpga, context,
            program, program_path, kernel_names, kernels, queues
    );

    // create buffers for CKS/CKR
    const int ports = 6;
    cl::Buffer routing_table_ck_s_0(context, CL_MEM_READ_ONLY, rank_count);
    cl::Buffer routing_table_ck_r_0(context, CL_MEM_READ_ONLY, ports*2);
    cl::Buffer routing_table_ck_s_1(context, CL_MEM_READ_ONLY, rank_count);
    cl::Buffer routing_table_ck_r_1(context, CL_MEM_READ_ONLY, ports*2);
    cl::Buffer routing_table_ck_s_2(context, CL_MEM_READ_ONLY, rank_count);
    cl::Buffer routing_table_ck_r_2(context, CL_MEM_READ_ONLY, ports*2);
    cl::Buffer routing_table_ck_s_3(context, CL_MEM_READ_ONLY, rank_count);
    cl::Buffer routing_table_ck_r_3(context, CL_MEM_READ_ONLY, ports*2);

    // load routing tables
    std::cout << "Using " << ports << " ports" << std::endl;
    char routing_tables_ckr[4][12 /* port count * 2 */];
    char routing_tables_cks[4][rank_count];
    for (int i = 0; i < 4; i++)
    {
        LoadRoutingTable<char>(rank, i, ports*2, routing_dir, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, routing_dir, "cks", &routing_tables_cks[i][0]);
    }

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE, 0, rank_count, &routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE, 0, ports*2, &routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE, 0, rank_count, &routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE, 0, ports*2, &routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE, 0, rank_count, &routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE, 0, ports*2, &routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE, 0, rank_count, &routing_tables_cks[3][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE, 0, ports*2, &routing_tables_ckr[3][0]);

    int num_ranks=rank_count;
    // cks_0
    kernels[0].setArg(0, sizeof(cl_mem), &routing_table_ck_s_0);
    kernels[0].setArg(1, sizeof(int), &num_ranks);

    // ckr_0
    kernels[1].setArg(0, sizeof(cl_mem), &routing_table_ck_r_0);
    kernels[1].setArg(1, sizeof(char), &rank);
    // cks_1
    kernels[2].setArg(0, sizeof(cl_mem), &routing_table_ck_s_1);
    kernels[2].setArg(1, sizeof(int), &num_ranks);

    // ckr_1
    kernels[3].setArg(0, sizeof(cl_mem), &routing_table_ck_r_1);
    kernels[3].setArg(1, sizeof(char), &rank);
    // cks_2
    kernels[4].setArg(0, sizeof(cl_mem), &routing_table_ck_s_2);
    kernels[4].setArg(1, sizeof(int), &num_ranks);

    // ckr_2
    kernels[5].setArg(0, sizeof(cl_mem), &routing_table_ck_r_2);
    kernels[5].setArg(1, sizeof(char), &rank);
    // cks_3
    kernels[6].setArg(0, sizeof(cl_mem), &routing_table_ck_s_3);
    kernels[6].setArg(1, sizeof(int), &num_ranks);

    // ckr_3
    kernels[7].setArg(0, sizeof(cl_mem), &routing_table_ck_r_3);
    kernels[7].setArg(1, sizeof(char), &rank);

    // broadcast 3
    kernels[8].setArg(0, sizeof(char), &rank_count);
    // broadcast 4
    kernels[9].setArg(0, sizeof(char), &rank_count);

    //start the kernels
    const int num_kernels = kernel_names.size();
    for(int i = num_kernels - 1; i >= 0; i--)
    {
        queues[i].enqueueTask(kernels[i]);
    }
}