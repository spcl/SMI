{%- macro impl_name_port_type(name, op) -%}{{ name }}_{{ op.logical_port }}_{{ op.data_type }}{%- endmacro -%}
