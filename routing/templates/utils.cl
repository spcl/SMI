{% macro table_array(key) %}{{ key ~ "_table" }}{% endmacro %}
{% macro channel_array(key) %}{{ key ~ "_channels" }}{% endmacro %}
{%- macro impl_name_port_type(name, op) -%}{{ name }}_{{ op.logical_port }}_{{ op.data_type }}{%- endmacro -%}
