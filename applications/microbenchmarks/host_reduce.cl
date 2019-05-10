//Host based implementation of the reduce
//- each rank prepare its data into DRAM
//- the root will also read again after the reduce has been done
//NOTICE that the read from memory are note unrolled, in order to be fair with
//the SMI implementation that produces one element at a clock-cycle

//each rank save data into memory
__kernel void app_rank(__global volatile int* memory,const int N)
{
    for(int i=0;i<N;i++)
    {
        memory[i]=2;
    }
}


//Read the reduce result from memory
__kernel void app_root(__global volatile int* memory,const int N)
{
    int maxv=0;
    for(int i=0;i<N;i++)
    {
        int data=memory[i];
        if(data>maxv) maxv=data;
    }
    memory[0]=maxv;

}
