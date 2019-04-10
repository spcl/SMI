/**
Host based broadcast
    In this case:
    - the application produces a stream of numbers to memory. To be consistent they are one element a time (in the broadcast
        the numbers are streamed)
    - the host copy from device and brodcast to remote
*/


//N is the total amount of data generated in memory
__kernel void app_0(__global float* memory,const int N)
{
    for(int i=0;i<N;i++)
    {
        memory[i]=1.1f;
    }
}
