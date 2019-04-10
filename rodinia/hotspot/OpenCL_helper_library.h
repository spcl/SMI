#ifndef OPENCL_HELPER_LIBRARY_H
#define OPENCL_HELPER_LIBRARY_H

#include "../common/opencl_util.h"

//#include <stdio.h>
//#include <sys/time.h>


// Function prototypes
//char *load_kernel_source(const char *filename);
//long long get_time();
#ifdef __cplusplus
extern "C" {
#endif
void fatal(const char *s);
void fatal_CL(cl_int error, int line_no);
#ifdef __cplusplus
}
#endif

#endif
