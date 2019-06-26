{% macro smi_init(program) -%}
#include <utils/smi_utils.hpp>

void SmiInit(
        const char rank,
        const char rank_count,
        const char* program_path,
        const char* routing_dir)
{
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    // channel kernels
    {% for channel in range(program.channel_count) %}
    kernel_names.push_back("smi_kernel_cks_{{ channel }}");
    kernel_names.push_back("smi_kernel_ckr_{{ channel }}");
    {% endfor %}

    // broadcast kernels
    {% for broadcast in program.get_broadcasts() %}
    kernel_names.push_back("smi_kernel_bcast_{{ broadcast.logical_port }}");
    {% endfor %}

    IntelFPGAOCLUtils::initEnvironment(
            platform, device, fpga, context,
            program, program_path, kernel_names, kernels, queues
    );

    // create buffers for CKS/CKR
    const int ports = {{ program.logical_port_count }};
    {% for channel in range(program.channel_count) %}
    cl::Buffer routing_table_ck_s_{{ channel }}(context, CL_MEM_READ_ONLY, rank_count);
    cl::Buffer routing_table_ck_r_{{ channel }}(context, CL_MEM_READ_ONLY, ports);
    {% endfor %}

    // load routing tables
    std::cout << "Using " << ports << " ports" << std::endl;
    char routing_tables_ckr[{{ program.channel_count}}][{{ program.logical_port_count * 2 }} /* port count * 2 */];
    char routing_tables_cks[{{ program.channel_count}}][rank_count];
    for (int i = 0; i < {{ program.channel_count }}; i++)
    {
        LoadRoutingTable<char>(rank, i, ports, routing_dir, "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(rank, i, rank_count, routing_dir, "cks", &routing_tables_cks[i][0]);
    }

    {% for channel in range(program.channel_count) %}
    queues[0].enqueueWriteBuffer(routing_table_ck_s_{{ channel }}, CL_TRUE, 0, rank_count, &routing_tables_cks[{{ channel }}][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_{{ channel }}, CL_TRUE, 0, ports, &routing_tables_ckr[{{ channel }}][0]);
    {% endfor %}

    {% set ctx = namespace(kernel=0) %}
    {% for channel in range(program.channel_count) %}
    // cks_{{ channel }}
    kernels[{{ ctx.kernel }}].setArg(0, sizeof(cl_mem), &routing_table_ck_s_{{ channel }});
    // ckr_{{ channel }}
    {% set ctx.kernel = ctx.kernel + 1 %}
    kernels[{{ ctx.kernel }}].setArg(0, sizeof(cl_mem), &routing_table_ck_r_{{ channel }});
    kernels[{{ ctx.kernel }}].setArg(1, sizeof(char), &rank);
    {% set ctx.kernel = ctx.kernel + 1 %}
    {% endfor %}

    {% for broadcast in program.get_broadcasts() %}
    // broadcast {{ broadcast.logical_port }}
    kernels[{{ ctx.kernel }}].setArg(0, sizeof(char), &rank_count);
    {% set ctx.kernel = ctx.kernel + 1 %}
    {% endfor %}

    //start the kernels
    const int num_kernels = kernel_names.size();
    for(int i = num_kernels - 1; i >= 0; i--)
    {
        queues[i].enqueueTask(kernels[i]);
    }
}
{%- endmacro %}
