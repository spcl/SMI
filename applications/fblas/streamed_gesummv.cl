/**
	Single modules have been code generated.
	I've put them here for convenience

        ATTENTION: helpers modified for better performance and achieve interleave
        //TODO
*/



#pragma OPENCL EXTENSION cl_intel_channels : enable

#define W 32
#define CHANNEL_VECTOR_X_A channel_x_A
#define CHANNEL_VECTOR_Y_A channel_vect_out_B
#define CHANNEL_VECTOR_OUT channel_vect_out_y
#define CHANNEL_MATRIX_A channel_matrix_A
#define CHANNEL_VECTOR_X_B channel_x_B
#define CHANNEL_VECTOR_Y_B channel_y_B
#define CHANNEL_VECTOR_OUT_B channel_vect_out_B
#define CHANNEL_MATRIX_B channel_matrix_B

#define INCX 1
#define INCY 1
#define INCW 1

#define TILE_N 1024
#define TILE_M 1024
#define __STRATIX_10__

#include <commons.h>
#if (TILE_N>TILE_M)
    #define MAX_TILE_SIZE TILE_N
#else
    #define MAX_TILE_SIZE TILE_M
#endif




channel TYPE_T CHANNEL_VECTOR_X_A __attribute__((depth(W)));
channel TYPE_T CHANNEL_VECTOR_Y_A __attribute__((depth(W)));
channel TYPE_T CHANNEL_MATRIX_A __attribute__((depth(W)));
channel TYPE_T CHANNEL_VECTOR_OUT __attribute__((depth(1)));
channel TYPE_T CHANNEL_VECTOR_X_B __attribute__((depth(W)));
channel TYPE_T CHANNEL_VECTOR_Y_B __attribute__((depth(W)));
channel TYPE_T CHANNEL_MATRIX_B __attribute__((depth(W)));

/**
    This version is meant for the following cases:
    - A is RowStreamed and NonTransposed, Tiles received by rows. In this case:
            - row_streamed must be set to 1
            - x is a vector of M elements, y is a vector of N elements
            - blocks of TILE_N elements of y are reused
            - also block of TILE_M elements of x are reused. The entire vector x must be resent N/TILE_N times (i.e. len_y/tile_y)
    - A is ColStreamed and Transposed, Tiles received by cols:
            - row_streamed must be set to 0
            - x is a vector of N elements, while y is a vector of M elements
            - blocks of y are of TILE_M elements
            - the entire x must be resent M/TILE_M times. Reuse will be applied also to it

    Matrix and vector must be padded to the respective tiling sizes
*/
__kernel void gemv_A(int row_streamed, const int N, const int M, const TYPE_T alpha, const TYPE_T beta)
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
    for(int ti=0;ti<BlocksY;ti++)
    {
        #pragma ivdep array(local_y)
        for(int tj=0;tj<BlocksX;tj++)
        {

            #pragma ivdep array(local_x)
            for(int i=0;i<tile_y;i++)
            {

                TYPE_T acc_o=0;
                TYPE_T acc_i=0;
                TYPE_T prev;


                //here we read one element from A and one element from X and we use it
                //For X we buffer it at the first iteration
                #pragma ivdep array(local_x)
                for(int jj=0;jj<computing_outer_loop_limit;jj++)
                {
                    if(tj==0 && jj==0) //read y if this is the first iteration over the blocks of X
                    {
                        if(beta==0)
                            prev=0;
                        else
                           prev=beta*read_channel_intel(CHANNEL_VECTOR_Y_A);
                    }
                    if(tj!=0)
                        prev=local_y[i];

                    if(i==0) //receive x
                    {
                        #pragma unroll
                        for(int j=0;j<W;j++)
                           local_x[jj*W+j]=read_channel_intel(CHANNEL_VECTOR_X_A);
                    }
                    acc_i=0;
                    //read (a block of W elements) of the row of A

                    //receive elemnts of a: decoupling this from the computation loop
                    //maybe usefulein case the sender of A does not perform unrolled writes into the channel
                    TYPE_T local_A[W];
                    #pragma unroll
                    for(int j=0;j<W;j++)
                        local_A[j]=read_channel_intel(CHANNEL_MATRIX_A);

                    #pragma unroll
                    for(int j=0;j<W;j++)
                        acc_i+=local_A[j]*local_x[jj*W+j];

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
                   write_channel_intel(CHANNEL_VECTOR_OUT,result);
            }
        }
    }
}




__kernel void gemv_B(int row_streamed, const int N, const int M, const TYPE_T alpha, const TYPE_T beta)
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
    for(int ti=0;ti<BlocksY;ti++)
    {
        #pragma ivdep array(local_y)
        for(int tj=0;tj<BlocksX;tj++)
        {

            #pragma ivdep array(local_x)
            for(int i=0;i<tile_y;i++)
            {

                TYPE_T acc_o=0;
                TYPE_T acc_i=0;
                TYPE_T prev;


                //here we read one element from A and one element from X and we use it
                //For X we buffer it at the first iteration
                #pragma ivdep array(local_x)
                for(int jj=0;jj<computing_outer_loop_limit;jj++)
                {
                    if(tj==0 && jj==0) //read y if this is the first iteration over the blocks of X
                    {
                        if(beta==0)
                            prev=0;
                        else
                           prev=beta*read_channel_intel(CHANNEL_VECTOR_Y_B);
                    }
                    if(tj!=0)
                        prev=local_y[i];

                    if(i==0) //receive x
                    {
                        #pragma unroll
                        for(int j=0;j<W;j++)
                           local_x[jj*W+j]=read_channel_intel(CHANNEL_VECTOR_X_B);
                    }
                    acc_i=0;
                    //read (a block of W elements) of the row of A

                    //receive elemnts of a: decoupling this from the computation loop
                    //maybe usefulein case the sender of A does not perform unrolled writes into the channel
                    TYPE_T local_A[W];
                    #pragma unroll
                    for(int j=0;j<W;j++)
                        local_A[j]=read_channel_intel(CHANNEL_MATRIX_B);

                    #pragma unroll
                    for(int j=0;j<W;j++)
                        acc_i+=local_A[j]*local_x[jj*W+j];

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
                   write_channel_intel(CHANNEL_VECTOR_OUT_B,result);
                }
            }
        }
    }
}




__kernel void read_matrix_A(__global volatile  TYPE_T *restrict data, int N, int M, unsigned int lda)
{
    const int BlocksN=1+(int)((N-1)/TILE_N);
    const int BlocksM=1+(int)((M-1)/TILE_M);
    const int outer_loop_limit=((int)(TILE_M))/W;   //W must be a divisor of TILE
    const int multiply64=1+(int)((lda-1)/64);

    TYPE_T to_send[W];
    for(int ti=0;ti<BlocksN;ti++)
    {
        for(int tj=0;tj<BlocksM;tj++)
        {
            for(int i = 0; i < TILE_N; i++)
            {
                for(int j=0; j < outer_loop_limit; j++ )
                {
                    #pragma unroll
                    for(int jj = 0; jj < W; jj++)
                    {
                        if((ti*TILE_N+i)<N  && tj*TILE_M+j*W+jj< M)
                        	 to_send[jj]=data[(ti*TILE_N+i)*64*multiply64+tj*TILE_M+j*W+jj];
                            //to_send[jj] = data[(ti*TILE_N+i)*lda+tj*TILE_M+j*W+jj];
                        else
                            to_send[jj]=0;
                    }

                    #pragma unroll
                    for(int jj = 0; jj < W; jj++)
                        write_channel_intel(CHANNEL_MATRIX_A,to_send[jj]);

                }
            }
        }
    }
}


__kernel void read_matrix_B(__global volatile TYPE_T *restrict data, int N, int M, unsigned int lda)
{
    const int BlocksN=1+(int)((N-1)/TILE_N);
    const int BlocksM=1+(int)((M-1)/TILE_M);
    const int outer_loop_limit=((int)(TILE_M))/W;   //W must be a divisor of TILE
    TYPE_T to_send[W];
        const int multiply64=1+(int)((lda-1)/64);

    for(int ti=0;ti<BlocksN;ti++)
    {
        for(int tj=0;tj<BlocksM;tj++)
        {
            for(int i = 0; i < TILE_N; i++)
            {
                for(int j=0; j < outer_loop_limit; j++ )
                {
                    #pragma unroll
                    for(int jj = 0; jj < W; jj++)
                    {
                        if((ti*TILE_N+i)<N  && tj*TILE_M+j*W+jj< M)
                        	 to_send[jj]=data[(ti*TILE_N+i)*64*multiply64+tj*TILE_M+j*W+jj];
                            //to_send[jj] = data[(ti*TILE_N+i)*lda+tj*TILE_M+j*W+jj];
                        else
                            to_send[jj]=0;
                    }

                    #pragma unroll
                    for(int jj = 0; jj < W; jj++)
                        write_channel_intel(CHANNEL_MATRIX_B,to_send[jj]);

                }
            }
        }
    }
}



__kernel void read_vector_x_A(__global volatile TYPE_T *restrict data, unsigned int N, unsigned int pad_size, unsigned int repetitions)
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
            }
            offset+=W*INCX;

            //send data
            #pragma unroll
            for(int k=0;k<W;k++)
                write_channel_intel(CHANNEL_VECTOR_X_A,x[k]);
        }
    }
}

__kernel void read_vector_x_B(__global volatile TYPE_T *restrict data, unsigned int N, unsigned int pad_size, unsigned int repetitions)
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
            }
            offset+=W*INCX;

            //send data
            #pragma unroll
            for(int k=0;k<W;k++)
                write_channel_intel(CHANNEL_VECTOR_X_B,x[k]);
        }
    }
}


__kernel void read_vector_y_B(__global volatile TYPE_T *restrict data, unsigned int N, unsigned int pad_size, unsigned int repetitions)
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

            //send data
            #pragma unroll
            for(int k=0;k<W;k++)
                write_channel_intel(CHANNEL_VECTOR_Y_B,y[k]);
        }
    }
}



__kernel void write_vector(__global volatile TYPE_T *restrict out, unsigned int N,unsigned int pad_size)
{
    const unsigned int ratio=pad_size/W;
    const unsigned int padding_loop_limit=ceil(((float)N)/pad_size);
    const unsigned int outer_loop_limit=padding_loop_limit*ratio;
    TYPE_T recv[W];
    //compute the starting index
    int offset=((INCW) > 0 ?  0 : ((N) - 1) * (-(INCW)));
    //receive and store data into memory
    for(int i=0;i<outer_loop_limit;i++)
    {
        #pragma unroll
        for(int j=0;j<W;j++)
        {
            recv[j]=read_channel_intel(CHANNEL_VECTOR_OUT);

            if(i*W+j<N)
                out[offset+(j*INCW)]=recv[j];
        }
        offset+=W*INCW;
    }
}

