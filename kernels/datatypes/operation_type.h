/**
  Definition of the supported communication operations
*/

#ifndef OPERATION_TYPE_H
#define OPERATION_TYPE_H
/**
    Type of operation performed
*/
typedef enum{
    SMI_SEND = 20,
    SMI_RECEIVE = 21,
    SMI_BROADCAST = 22
}SMI_Operationtype;

#endif
