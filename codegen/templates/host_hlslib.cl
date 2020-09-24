#define __HOST_PROGRAM__
#include <hlslib/intel/OpenCL.h>
#include <utils/smi_utils.hpp>
#include <vector>
#include <smi/communicator.h>

{% for (name, program) in programs -%}
SMI_Comm SmiInit_{{ name }}(
        int rank,
        int ranks_count,
        const char* routing_dir,
        hlslib::ocl::Context &context,
        hlslib::ocl::Program &program,
        std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>> &buffers)
{

    const int ports = {{ program.logical_port_count }};
    const int cks_table_size = ranks_count * ports;
    const int ckr_table_size = ports * 2;
    // load routing tables
    std::vector<std::vector<char>> routing_tables_ckr({{ program.channel_count}}, std::vector<char>(ckr_table_size));
    std::vector<std::vector<char>> routing_tables_cks({{ program.channel_count}}, std::vector<char>(cks_table_size));
    for (int i = 0; i < {{ program.channel_count }}; i++)
    {
        LoadRoutingTable<char>(rank, i, cks_table_size, routing_dir, "cks", &routing_tables_cks[i][0]);
        LoadRoutingTable<char>(rank, i, ckr_table_size, routing_dir, "ckr", &routing_tables_ckr[i][0]);
    }

    // create buffers for CKS/CKR and copy routing tables
    {% for channel in range(program.channel_count) %}
    hlslib::ocl::Buffer<char, hlslib::ocl::Access::read> routing_table_device_ck_s_{{ channel }} =
        context.MakeBuffer<char, hlslib::ocl::Access::read>( routing_tables_cks[{{channel}}].cbegin(),
        routing_tables_cks[{{channel}}].cend());

    hlslib::ocl::Buffer<char, hlslib::ocl::Access::read> routing_table_device_ck_r_{{ channel }} =
        context.MakeBuffer<char, hlslib::ocl::Access::read>( routing_tables_ckr[{{channel}}].cbegin(),
        routing_tables_ckr[{{channel}}].cend());
    {% endfor %}


    char char_ranks_count=ranks_count;
    char char_rank=rank;

    // CK kernels
    std::vector<hlslib::ocl::Kernel> comm_kernels;
    {% for channel in range(program.channel_count) %}
    // cks_{{ channel }}
    comm_kernels.emplace_back(program.MakeKernel("smi_kernel_cks_{{ channel }}", routing_table_device_ck_s_{{channel}}, (char)char_ranks_count));

    // ckr_{{ channel }}
    comm_kernels.emplace_back(program.MakeKernel("smi_kernel_ckr_{{ channel }}", routing_table_device_ck_r_{{ channel }}, (char)char_rank));

    {% endfor %}

    // Collective kernels
    std::vector<hlslib::ocl::Kernel> collective_kernels;
    {%- macro generate_collective_kernels(key, kernel_name) %}
    {% set ops = program.get_ops_by_type(key) %}
    {% for op in ops %}
    // {{ key }} {{ op.logical_port }}
    collective_kernels.emplace_back(program.MakeKernel("{{ kernel_name }}_{{ op.logical_port }}", (char)char_ranks_count));
    {% endfor %}
    {%- endmacro %}

    {{ generate_collective_kernels("broadcast", "smi_kernel_bcast") }}
    {{ generate_collective_kernels("reduce", "smi_kernel_reduce") }}
    {{ generate_collective_kernels("scatter", "smi_kernel_scatter") }}
    {{ generate_collective_kernels("gather", "smi_kernel_gather") }}

    // start the kernels
    for (auto &k : comm_kernels) {
      // Will never terminate, so we don't care about the return value of fork
      k.ExecuteTaskFork();
    }
    
    for (auto &k : collective_kernels) {
      // Will never terminate, so we don't care about the return value of fork
      k.ExecuteTaskFork();
    }

    //move created buffers to the vector given my the user
    {% for channel in range(program.channel_count) %}
    buffers.push_back(std::move(routing_table_device_ck_s_{{channel}}));
    buffers.push_back(std::move(routing_table_device_ck_r_{{channel}}));
    {% endfor %}

    // return the communicator
    SMI_Comm comm{ char_rank, char_ranks_count };
    return comm;

}
{% endfor %}
