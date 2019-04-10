//Bandwdith for host based Bandwdith
//In this case: 
//- the application produces a stream of numbers to memory. To be consistent they are stored three elem at a time
//- the host copy from device and sends to remote
//notice that the fact that we are using a single memory module is not a problem since it is clearly faster
//than the network
#pragma OPENCL EXTENSION cl_khr_fp64 : enable


//N is the total amount of data generated in memory
__kernel void app_0(__global double* memory,const int N)
{
	const int loop_limit=N/3;
    for(int i=0;i<loop_limit;i++)
    {
        memory[i*3]=1.1f;
        memory[i*3+1]=1.1f;
        memory[i*3+2]=1.1f;
    }

}