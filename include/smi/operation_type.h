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
    SMI_SYNCH=3,        //special operation type used for synchronization/rendezvou
    SMI_SCATTER=4,
    SMI_REDUCE=5,
    SMI_GATHER=6
}SMI_Operationtype;

#endif
