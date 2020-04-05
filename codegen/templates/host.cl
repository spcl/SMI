#define __HOST_PROGRAM__
#include <utils/smi_utils.hpp>
#include <vector>
#include <smi/communicator.h>

{% for (name, program) in programs -%}
SMI_Comm SmiInit_{{ name }}(
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
    {% for channel in range(program.channel_count) %}
    kernel_names.push_back("smi_kernel_cks_{{ channel }}");
    kernel_names.push_back("smi_kernel_ckr_{{ channel }}");
    {% endfor %}
    {%- macro generate_collective_kernels(key, kernel_name) %}
    {% set ops = program.get_ops_by_type(key) %}
    // {{ key }} kernels
    {% for op in ops %}
    kernel_names.push_back("{{ kernel_name }}_{{ op.logical_port }}");
    {% endfor %}
    {%- endmacro %}

    {{ generate_collective_kernels("broadcast", "smi_kernel_bcast") }}
    {{ generate_collective_kernels("reduce", "smi_kernel_reduce") }}
    {{ generate_collective_kernels("scatter", "smi_kernel_scatter") }}
    {{ generate_collective_kernels("gather", "smi_kernel_gather") }}

    IntelFPGAOCLUtils::initEnvironment(
            platform, device, fpga, context,
            program, program_path, kernel_names, kernels, queues
    );

    // create buffers for CKS/CKR
    const int ports = {{ program.logical_port_count }};
    const int cks_table_size = ranks_count * ports;
    const int ckr_table_size = ports * 2;
    {% for channel in range(program.channel_count) %}
    cl::Buffer routing_table_ck_s_{{ channel }}(context, CL_MEM_READ_ONLY, cks_table_size);
    cl::Buffer routing_table_ck_r_{{ channel }}(context, CL_MEM_READ_ONLY, ckr_table_size);
    {% endfor %}

    // load routing tables
    char routing_tables_cks[{{ program.channel_count}}][cks_table_size];
    char routing_tables_ckr[{{ program.channel_count}}][ckr_table_size];
    for (int i = 0; i < {{ program.channel_count }}; i++)
    {
        LoadRoutingTable<char>(rank, i, cks_table_size, routing_dir, "cks", &routing_tables_cks[i][0]);
        LoadRoutingTable<char>(rank, i, ckr_table_size, routing_dir, "ckr", &routing_tables_ckr[i][0]);
    }

    {% for channel in range(program.channel_count) %}
    queues[0].enqueueWriteBuffer(routing_table_ck_s_{{ channel }}, CL_TRUE, 0, cks_table_size, &routing_tables_cks[{{ channel }}][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_{{ channel }}, CL_TRUE, 0, ckr_table_size, &routing_tables_ckr[{{ channel }}][0]);
    {% endfor %}

    char char_ranks_count=ranks_count;
    char char_rank=rank;
    {% set ctx = namespace(kernel=0) %}
    {% for channel in range(program.channel_count) %}
    // cks_{{ channel }}
    kernels[{{ ctx.kernel }}].setArg(0, sizeof(cl_mem), &routing_table_ck_s_{{ channel }});
    kernels[{{ ctx.kernel }}].setArg(1, sizeof(char), &char_ranks_count);

    // ckr_{{ channel }}
    {% set ctx.kernel = ctx.kernel + 1 %}
    kernels[{{ ctx.kernel }}].setArg(0, sizeof(cl_mem), &routing_table_ck_r_{{ channel }});
    kernels[{{ ctx.kernel }}].setArg(1, sizeof(char), &char_rank);
    {% set ctx.kernel = ctx.kernel + 1 %}
    {% endfor %}

    {%- macro setup_collective_kernels(key) %}
    {% set ops = program.get_ops_by_type(key) %}
    {% for op in ops %}
    // {{ key }} {{ op.logical_port }}
    kernels[{{ ctx.kernel }}].setArg(0, sizeof(char), &char_ranks_count);
    {% set ctx.kernel = ctx.kernel + 1 %}
    {% endfor %}
    {%- endmacro %}
    {{ setup_collective_kernels("broadcast") }}
    {{ setup_collective_kernels("reduce") }}
    {{ setup_collective_kernels("scatter") }}
    {{ setup_collective_kernels("gather") }}

    // move buffers
    {% for channel in range(program.channel_count) %}
    buffers.push_back(std::move( routing_table_ck_s_{{ channel }}));
    buffers.push_back(std::move( routing_table_ck_r_{{ channel }}));
    {% endfor %}

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
{% endfor %}
