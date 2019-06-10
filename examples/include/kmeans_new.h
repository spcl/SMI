#pragma once

#pragma OPENCL EXTENSION cl_intel_channels : enable

#define K 8
#define DIMS 64
#define DTYPE float
#define ITYPE unsigned short
#define SMI_TYPE SMI_FLOAT
#define W 16
#if W > 1
#define VTYPE float16
#define IVTYPE ushort16
#else
#define VTYPE DTYPE
#define IVTYPE ITYPE
#endif
#define SMI_DEVICES_PER_NODE 2
