/**
  Definition of the supported communication operations
*/

#ifndef OPERATION_TYPE_H
#define OPERATION_TYPE_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
/**
    Type of operation performed
*/
typedef enum{
    SMI_SEND = 0,
    SMI_RECEIVE = 1,
    SMI_BROADCAST = 2,
    SMI_REQUEST=3
}SMI_Operationtype;

#endif
