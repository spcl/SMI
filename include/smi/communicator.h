#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H
/**
  @file communicator.h
  Describes a basic communicator.
*/

//Note: Since the Intel compiler fails in compiling the emulation if you pass a user-defined
//data type, we had to define it by resorting to OpenCL data types: the first element
//will be "my_rank" and the second the number of ranks
#if defined __HOST_PROGRAM__
typedef cl_char2 SMI_Comm;
#else
typedef char2 SMI_Comm;
/**
 * @brief SMI_Comm_size return the communicator size
 * @param comm
 * @return the communicator size
 */
inline int SMI_Comm_size(SMI_Comm comm){
    return comm[1];
}

/**
 * @brief SMI_Comm_rank determins the rank of the caller
 * @param comm
 * @return rank of the caller
 */
inline int SMI_Comm_rank(SMI_Comm comm){
    return comm[0];
}
#endif
#endif
