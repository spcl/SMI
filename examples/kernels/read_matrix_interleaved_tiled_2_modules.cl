/**
        The kernels read the matrix, which is interleaved
        among different modules (2 modules)


        Elements are interleaved with size 16 float elements (64 bytes)
        We impose that M is a multiple of 64, so we know that a row starts
        always from the first module.

        In this version we consider also tiling, by rows with element row-streamed.

*/


#pragma OPENCL EXTENSION cl_intel_channels : enable
#define TILE_N 128
#define TILE_M 128
#define W 64 //do not change this

channel float channel_matrix_A __attribute__((depth(W)));


//N and M must be a multiple of 64
__kernel void read_matrix_A(__global volatile  float *restrict data0,__global volatile  float *restrict data1, int N, int M, unsigned int lda)
{
    const int BlocksN=1+(int)((N-1)/TILE_N);
    const int BlocksM=1+(int)((M-1)/TILE_M);
    const int loop_it=((int)(TILE_M))/W;   //W must be a divisor of TILE
    const int multiply64=1+(int)((lda-1)/64); //lda must be a multiple of 64, otherwise inefficient hw is generated for the load

    float to_send[W];

    for(int ti=0;ti<BlocksN;ti++)
    {
        for(int tj=0;tj<BlocksM;tj++)
        {
            for(int i=0;i<TILE_N;i++)
            {
                for(int j=0;j<loop_it;j++)
                {
                    const int row_idx=ti*TILE_N+i;
                    //load from memory

                    #pragma unroll
                    for(int k=0;k<32;k++)
                            to_send[k]=data0[row_idx*32*multiply64+tj*TILE_M/2+j*32+k];

                    #pragma unroll
                    for(int k=0;k<32;k++)
                            to_send[k+32]=data1[row_idx*32*multiply64+tj*TILE_M/2+j*32+k];

                    #pragma unroll
                    for(int k = 0; k < W; k++)
                        write_channel_intel(channel_matrix_A,to_send[k]);
                }
            }
        }
    }
}



__kernel void receiver(__global volatile  float *restrict mem, int N, int M)
{
    const int BlocksN=1+(int)((N-1)/TILE_N);
    const int BlocksM=1+(int)((M-1)/TILE_M);
    const int loop_it=((int)(TILE_M))/W;   //W must be a divisor of M
    float  sum=0;
    float recv[W];
    //float expected=0.0f;
    //int errors=0;

    for(int ti=0;ti<BlocksN;ti++)
    {
        float summ_i=0; //i've done this intermediate accumulation because with 18.0.1 it complains about dependencies
        for(int tj=0;tj<BlocksM;tj++)
        {
            for(int i=0;i<TILE_N;i++)
            {
                for(int j=0;j<loop_it;j++)
                {
                    float acc=0;
                    #pragma unroll
                    for(int k = 0; k < W; k++)
                    {
                        recv[k]=read_channel_intel(channel_matrix_A);
                        acc+=recv[k];
                        /*expected=(ti*TILE_N+i)*M+tj*TILE_M+j*W+k;
                        if(recv[k]!=expected && errors <16)
                        {
                            printf("Error on element %d, %d (%.1f != %.1f) \n",ti*TILE_N+i,tj*TILE_M+j*W+k,recv[k],expected);
                            errors++;
                        }*/
                    }
                    summ_i+=acc;
                }
            }
        }
        sum+=summ_i;
    }

   *mem=sum;
}
