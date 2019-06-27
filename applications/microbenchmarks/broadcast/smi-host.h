
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
    // broadcast kernels
    kernel_names.push_back("smi_kernel_bcast_0");

    kernel_names.push_back("smi_kernel_cks_0");
    kernel_names.push_back("smi_kernel_cks_1");
    kernel_names.push_back("smi_kernel_cks_2");
    kernel_names.push_back("smi_kernel_cks_3");
    kernel_names.push_back("smi_kernel_ckr_0");
    kernel_names.push_back("smi_kernel_ckr_1");
    kernel_names.push_back("smi_kernel_ckr_2");
    kernel_names.push_back("smi_kernel_ckr_3");



    IntelFPGAOCLUtils::initEnvironment(
            platform, device, fpga, context,
            program, program_path, kernel_names, kernels, queues
    );

    // create buffers for CKS/CKR
     char tags=1;
    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,rank_count);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags*2);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags*2);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags*2);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags*2);


    //load ck_r
    std::cout << "Using "<<tags<<" tags"<<std::endl;
    char routing_tables_ckr[4][tags*2]; //only one tag
    char routing_tables_cks[4][rank_count]; //4 ranks
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(rank, i, tags*2, routing_dir, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, routing_dir, "cks", &routing_tables_cks[i][0]);
    }
    //std::cout << "Rank: "<< rank<<endl;
   // for(int i=0;i<kChannelsPerRank;i++)
    //    for(int j=0;j<rank_count;j++)
    //        std::cout << i<< "," << j<<": "<< (int)routing_tables_cks[i][j]<<endl;

    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,rank_count,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,rank_count,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,rank_count,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,rank_count,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,tags*2,&routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,tags*2,&routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,tags*2,&routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,tags*2,&routing_tables_ckr[3][0]);


    kernels[0].setArg(0,sizeof(char),&rank_count);


    //args for the CK_Ss
    int num_ranks=rank_count;
    kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[1].setArg(1,sizeof(int),&num_ranks);
    kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[2].setArg(1,sizeof(int),&num_ranks);
    kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[3].setArg(1,sizeof(int),&num_ranks);
    kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);
    kernels[4].setArg(1,sizeof(int),&num_ranks);

    //args for the CK_Rs
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[5].setArg(1,sizeof(char),&rank);
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[6].setArg(1,sizeof(char),&rank);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[7].setArg(1,sizeof(char),&rank);
    kernels[8].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[8].setArg(1,sizeof(char),&rank);

    //start the CKs
    const int num_kernels=kernel_names.size();

    for(int i=0;i<num_kernels;i++) //start also the bcast kernel
        queues[i].enqueueTask(kernels[i]);
}