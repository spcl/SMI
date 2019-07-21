/**
    Distributed implementation of GESUMMV

    RANK-1 will compute y''=beta*A*x.
    This, will be send to RANK-0
    for producing the final result

    GEMV has access to 2 memory modules.

*/


#pragma OPENCL EXTENSION cl_intel_channels : enable

#define W 64
#define TILE_N 128
#define TILE_M 128
#define INCX 1
#define INCY 1
#define INCW 1
#define KERNEL_NAME gemv
#define CHANNEL_VECTOR_X channel_in_vector_x_0
#define CHANNEL_VECTOR_Y channel_in_vector_y_0
#define CHANNEL_MATRIX_A channel_in_matrix_A_0
#define CHANNEL_RESULT channel_result
#define READ_VECTOR_X kernel_read_vector_x_0
#define READ_VECTOR_Y kernel_read_vector_y_0
#define READ_MATRIX_A kernel_read_matrix_A_0
#define __STRATIX_10__
#include <smi.h>
#include <fblas.h>
#if (TILE_N>TILE_M)
    #define MAX_TILE_SIZE TILE_N
#else
    #define MAX_TILE_SIZE TILE_M
#endif


channel TYPE_T CHANNEL_VECTOR_X __attribute__((depth(W)));
channel TYPE_T CHANNEL_VECTOR_Y __attribute__((depth(W)));
channel TYPE_T CHANNEL_MATRIX_A __attribute__((depth(W)));
channel TYPE_T CHANNEL_RESULT __attribute__((depth(W)));


__kernel void KERNEL_NAME(int row_streamed, const int N, const int M, TYPE_T alpha, const TYPE_T beta, SMI_Comm comm)
{
    int len_x,tile_x;
    int len_y,tile_y;
    int BlocksX, BlocksY;
    //chose the loop limits
    if(row_streamed == 1)
    {
        len_x = M;
        len_y = N;
        tile_x=TILE_M;
        tile_y=TILE_N;
        BlocksY=1+(int)((N-1)/TILE_N); //ceiling
        BlocksX=1+(int)((M-1)/TILE_M);
    }
    else
    {	//in this case A is transposed
        len_x = N;
        len_y = M;
        tile_x=TILE_N;
        tile_y=TILE_M;
        BlocksY=1+(int)((M-1)/TILE_M);
        BlocksX=1+(int)((N-1)/TILE_N);
    }

    #ifdef DOUBLE_PRECISION
    TYPE_T shift_reg[SHIFT_REG+1]; //shift register

    for(int i=0;i<SHIFT_REG+1;i++)
       shift_reg[i]=0;
    #endif


    //The computation is performed by receiving A in tiles by row (A non transposed) or column (A transposed).
    //In this way, the result is computed by 'accumulating' over y elements
    //One block of y is computed for each row-tile (or column-tile) of A and using the entire x

    const int computing_outer_loop_limit=(int)(tile_x/W);
    const int reading_y_outer_loop_limit=(int)(tile_y/W);

    //Please note: the order in which tiles arrive, will determine the computation
    //(i.e. do not assume that you will receive the tiles one row after the other...maybe they can arrive column by column)

    TYPE_T local_y[MAX_TILE_SIZE];
    TYPE_T local_x[MAX_TILE_SIZE];
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_FLOAT,0,0,comm);
    #pragma max_concurrency 1
    #pragma loop_coalesce 2
    for(int ti=0;ti<BlocksY;ti++)
    {
       //#pragma ivdep array(local_y)
        #pragma max_concurrency 1
        for(int tj=0;tj<BlocksX;tj++)
        {

            //#pragma ivdep array(local_x)
            for(int i=0;i<tile_y;i++)
            {

                TYPE_T acc_o=0;
                TYPE_T acc_i=0;
                TYPE_T prev;


                //here we read one element from A and one element from X and we use it
                //For X we buffer it at the first iteration
                //#pragma ivdep array(local_x)
                for(int jj=0;jj<computing_outer_loop_limit;jj++)
                {
                    if(tj==0 && jj==0) //read y if this is the first iteration over the blocks of X
                    {
                        if(beta==0)
                            prev=0;
                        else
                           prev=beta*read_channel_intel(CHANNEL_VECTOR_Y);
                    }
                    if(tj!=0)
                        prev=local_y[i];

                    if(i==0) //receive x
                    {
                        #pragma unroll
                        for(int j=0;j<W;j++)
                        {
                           local_x[jj*W+j]=read_channel_intel(CHANNEL_VECTOR_X);
                        }
                    }
                    acc_i=0;
                    //read (a block of W elements) of the row of A

                    //receive elemnts of a: decoupling this from the computation loop
                    //maybe usefulein case the sender of A does not perform unrolled writes into the channel
                    TYPE_T local_A[W];
                    #pragma unroll
                    for(int j=0;j<W;j++)
                    {
                        local_A[j]=read_channel_intel(CHANNEL_MATRIX_A);
                    }

                    #pragma unroll
                    for(int j=0;j<W;j++)
                    {
                        acc_i+=local_A[j]*local_x[jj*W+j];
                    }

                    #ifdef DOUBLE_PRECISION
                        shift_reg[SHIFT_REG] = shift_reg[0]+alpha*acc_i;
                        //Shift every element of shift register
                        #pragma unroll
                        for(int j = 0; j < SHIFT_REG; ++j)
                            shift_reg[j] = shift_reg[j + 1];
                    #else
                        acc_o+=alpha*acc_i;
                    #endif

                }
                #ifdef DOUBLE_PRECISION
                    //reconstruct the result using the partial results in shift register
                    #pragma unroll
                    for(int i=0;i<SHIFT_REG;i++)
                    {
                        acc_o+=shift_reg[i];
                        shift_reg[i]=0;
                    }
                #endif
                TYPE_T result =  prev+ acc_o;
                local_y[i] = result;

                //output y if we reached the end of the matrix
                //y is output one element at a time
                if(tj==BlocksX-1)
                {
                    SMI_Push(&chan,&result);
                }
            }
        }
    }
}



__kernel void READ_VECTOR_X(__global volatile TYPE_T *restrict data, unsigned int N, unsigned int pad_size, unsigned int repetitions)
{
    unsigned int ratio=pad_size/W;

    unsigned int padding_loop_limit=ceil(((float)N)/pad_size);
    unsigned int outer_loop_limit=padding_loop_limit*ratio;
    TYPE_T x[W];
    for(int t=0; t< repetitions;t++)
    {
        //compute the starting index
        int offset=((INCX) > 0 ?  0 : ((N) - 1) * (-(INCX)));

        for(int i=0;i<outer_loop_limit;i++)
        {
            //prepare data
            #pragma unroll
            for(int k=0;k<W;k++)
            {
                if(i*W+k<N)
                    x[k]=data[offset+(k*INCX)];
                else
                    x[k]=0;
                write_channel_intel(CHANNEL_VECTOR_X,x[k]);
            }
            offset+=W*INCX;

        }
    }
}


__kernel void READ_VECTOR_Y(__global  volatile TYPE_T *restrict data, unsigned int N, unsigned int pad_size, unsigned int repetitions)
{

    unsigned int ratio=pad_size/W;
    unsigned int padding_loop_limit=ceil(((float)N)/pad_size);
    unsigned int outer_loop_limit=padding_loop_limit*ratio;
    TYPE_T y[W];
    for(int t=0; t< repetitions;t++)
    {
        //compute the starting index
        int offset=((INCY) > 0 ?  0 : ((N) - 1) * (-(INCY)));
        for(int i=0;i<outer_loop_limit;i++)
        {
            //prepare data
            #pragma unroll
            for(int k=0;k<W;k++)
            {
                if(i*W+k<N)
                    y[k]=data[offset+(k*INCY)];
                else
                    y[k]=0;
            }
            offset+=W*INCY;
            #pragma unroll
            for(int k=0;k<W;k++)
                write_channel_intel(CHANNEL_VECTOR_Y,y[k]);
        }
    }
}



//N and M must be a multiple of 64
__kernel void READ_MATRIX_A(__global volatile  float *restrict data0,__global volatile  float *restrict data1, int N, int M, unsigned int lda)
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
                        write_channel_intel(CHANNEL_MATRIX_A,to_send[k]);
                }
            }
        }
    }
}






