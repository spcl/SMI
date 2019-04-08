/**
    Broadcast host based benchmar
    In this case:
    - the host copy to the device the data arriving from the remote
    - the application reads the data from memory
*/


//N is the total amount of data generated in memory
__kernel void app_0(__global float* memory,const int N)
{
    float maxv=0;
    for(int i=0;i<N;i++)
    {
        float data=memory[i];
        if(data>maxv) maxv=data;
    }
    memory[0]=maxv;

}
