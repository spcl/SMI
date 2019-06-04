#pragma once

#pragma OPENCL EXTENSION cl_intel_channels : enable

#define K 8
#define DIMS 8
#define DTYPE float
#define ITYPE unsigned short
#define SMI_TYPE SMI_FLOAT
#define W 4
#if W > 1
#define VTYPE float4
#define IVTYPE ushort4
#else
#define VTYPE DTYPE
#define IVTYPE ITYPE
#endif
#define SMI_DEVICES_PER_NODE 2
