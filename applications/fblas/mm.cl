//Automatically generated file

#pragma OPENCL EXTENSION cl_intel_channels : enable

#define CTILE_ROWS 32
#define CTILE_COLS 16
#define MTILE 32
#define KERNEL_NAME sgemm
#define CHANNEL_MATRIX_A channel_in_matrix_A_0
#define CHANNEL_MATRIX_B channel_in_matrix_B_0
#define CHANNEL_MATRIX_OUT channel_out_matrix_0
#define READ_MATRIX_A read_matrix_A
#define READ_MATRIX_B read_matrix_B
#define WRITE_MATRIX write_matrix
#define __STRATIX_10__

#include <commons.h>

channel TYPE_T CHANNEL_MATRIX_A __attribute__((depth(CTILE_ROWS)));
channel TYPE_T CHANNEL_MATRIX_B __attribute__((depth(CTILE_COLS)));
channel TYPE_T CHANNEL_MATRIX_OUT __attribute__((depth(CTILE_COLS)));


/**
  Computes the GEMM. A and B are received through input channels.
    - Matrix A: for each outer tile (MTILE size), inner blocks are received one after the other
            The entire outer tile-row (MTILE x K) is resent a number of times equal to the number of
            outer tiles in matrix B (check helpers/read_matrix_a_notrans_gemm.cl for an example)
    - Matrix B: for each outer tile, inner blocks are received one of the other. Each
        outer tile row is sent multiple times (check helpers/read_matrx_b_notrans_gemm.cl for an example)

*/
__kernel void KERNEL_NAME(const int N, const int M, const int K, const TYPE_T alpha)

{

    const int OuterBlocksN = 1 + (int)((N-1) / MTILE);
    const int OuterBlocksM = 1 + (int)((M-1) / MTILE);
    const int InnerBlocksN = MTILE / CTILE_ROWS;
    const int InnerBlocksM = MTILE / CTILE_COLS;
    __local TYPE_T __attribute__((memory)) localC[MTILE/CTILE_ROWS][MTILE/CTILE_COLS][CTILE_ROWS][CTILE_COLS];

    //GEMM with outer product

    //#pragma unroll
    for(int ti=0;ti<OuterBlocksN;ti++)	    //outer tile
    {
        for(int tj=0;tj<OuterBlocksM;tj++) //outer tile
        {

            for(int k=0;k<K;k++)
            {
                TYPE_T localA[CTILE_ROWS];
                TYPE_T localB[CTILE_COLS];
                #pragma unroll 1
                for(int tti=0;tti<InnerBlocksN;tti++)   //inner tile
                {
                   // #pragma unroll
                   // for(int i=0;i<CTILE_ROWS;i++)
                   //     localA[i]=read_channel_intel(CHANNEL_MATRIX_A);
                    #pragma unroll 1
                    for(int ttj=0;ttj<InnerBlocksM; ttj++)	//inner tile
                    {
                        if(ttj==0)
                        {
                            #pragma unroll
                            for(int i=0;i<CTILE_ROWS;i++)
                              localA[i]=read_channel_intel(CHANNEL_MATRIX_A);
                        }
                        #pragma unroll
                        for(int i=0;i<CTILE_COLS;i++)
                            localB[i]=read_channel_intel(CHANNEL_MATRIX_B);

                        //to unroll
                        #pragma unroll
                        for(int i=0;i<CTILE_ROWS;i++)
                        {
                            TYPE_T tmpa=alpha*localA[i];
                            #pragma unroll
                            for(int j=0; j<CTILE_COLS;j++)
                            {

                                const TYPE_T prev=(k==0)?0:localC[tti][ttj][i][j];
                                localC[tti][ttj][i][j]=prev+tmpa*localB[j];
                            }
                        }
                    }
                }
            }

            //prevent unrolls on this, for the sake of saving BRAMs
            #pragma unroll 1
            for(int ii=0;ii<MTILE/CTILE_ROWS;ii++)
                #pragma unroll 1
                for(int jj=0;jj<MTILE/CTILE_COLS;jj++)
                {
                    #pragma unroll 1
                    for(int iii=0;iii<CTILE_ROWS;iii++)
                            #pragma unroll
                            for(int jjj=0;jjj<CTILE_COLS;jjj++)
                            {
                                    int ind_i=ti*MTILE+ii*CTILE_ROWS+iii;
                                    int ind_j=tj*MTILE+jj*CTILE_COLS+jjj;
                                    write_channel_intel(CHANNEL_MATRIX_OUT,localC[ii][jj][iii][jjj]);
                             }
                }
        }
    }


}





__kernel void READ_MATRIX_A(__global volatile TYPE_T * restrict A, const unsigned int N, const unsigned int K, const unsigned int M, const unsigned int lda)
{
    //double level of tiling
    const int OuterBlocksN = 1 + (int)((N-1) / MTILE);
    const int OuterBlocksM = 1 + (int)((M-1) / MTILE);
    const int InnerBlocksN = MTILE / CTILE_ROWS;
    const int InnerBlocksM = MTILE / CTILE_COLS;
    const int BlocksK=(int)(K/MTILE);

    TYPE_T localA[MTILE];
    for(int ti=0; ti< OuterBlocksN;ti++)
    {

        //resend this tile a number of times equal to the number of column tiles of the matrix B
        for(int tj=0;tj<OuterBlocksM;tj++)
        {
            for(int k=0;k<K;k++)
            {
                //load A
                #pragma unroll 8
                for(int i=0;i<MTILE;i++)
                {
                    if(ti*MTILE+i < N)
                        localA[i]=A[(ti*MTILE+i)*lda+k];
                    else
                        localA[i]=0;
                }

                //now we have to iterates over the inner tiles of size CTILE_ROWS x MTILE
                //each of them will be sent only once (and will be reused InnerBlocksM times)
                for(int tti=0; tti<InnerBlocksN;tti++)
                {

                    #pragma unroll
                    for(int i=0;i<CTILE_ROWS;i++)
                    {
                        write_channel_intel(CHANNEL_MATRIX_A,localA[tti*CTILE_ROWS+i]);
                    }
                }
            }
        }
    }
}

__kernel void READ_MATRIX_B(__global volatile TYPE_T * restrict B, const unsigned int N, const unsigned int K, const unsigned int M, const unsigned int ldb)
{

    const int OuterBlocksN = 1 + (int)((N-1) / MTILE);
    const int OuterBlocksM = 1 + (int)((M-1) / MTILE);
    const int InnerBlocksN = MTILE / CTILE_ROWS;
    const int InnerBlocksM = MTILE / CTILE_COLS;

    TYPE_T localB[MTILE];

    for(int ti=0;ti<OuterBlocksN;ti++)
    {
        //outer tile over columns of B
        for(int tj=0;tj<OuterBlocksM;tj++)
        {
            for(int k=0;k<K;k++)
            {
                #pragma unroll 16
                for(int i=0;i<MTILE;i++)
                {
                    if(tj*MTILE+i < M)
                        localB[i]=B[k*ldb+(tj*MTILE+i)];
                    else
                        localB[i]=0;
                }
                for(int i=0;i<InnerBlocksN;i++)
                {
                    //iterates over the inner tiles of B
                    for(int ttj=0;ttj<InnerBlocksM;ttj++)
                    {
                        #pragma unroll
                        for(int j=0;j<CTILE_COLS;j++)
                        {
                            write_channel_intel(CHANNEL_MATRIX_B,localB[ttj*CTILE_COLS+j]);
                        }
                    }
                }
            }
        }
    }
}

__kernel void WRITE_MATRIX(__global volatile TYPE_T * restrict C, const TYPE_T beta,const unsigned int N, const unsigned int M, const unsigned int ldc)
{
    //this kernel will receive the data for C in order
    const int OuterBlocksN = 1 + (int)((N-1) / MTILE);
    const int OuterBlocksM = 1 + (int)((M-1) / MTILE);
    const int InnerBlocksN = MTILE / CTILE_ROWS;
    const int InnerBlocksM = MTILE / CTILE_COLS;

    //__local TYPE_T localC[MTILE][MTILE/CTILE_COLS][CTILE_COLS];


    //for each outer tile of C..
    for(int ti=0;ti<OuterBlocksN;ti++)
    {
        for(int tj=0;tj<OuterBlocksM;tj++)
        {

            //read and save
             #pragma unroll 1
            #pragma ivdep array(C)
             for(int ii=0;ii<MTILE/CTILE_ROWS;ii++)
                 #pragma unroll 1
                 #pragma ivdep array(C)
                 for(int jj=0;jj<MTILE/CTILE_COLS;jj++)
                 {
                     #pragma unroll 1
                     #pragma max_concurrency 1
                     #pragma ivdep array(C)
                     for(int iii=0;iii<CTILE_ROWS;iii++)
                         #pragma unroll
                         for(int jjj=0;jjj<CTILE_COLS;jjj++)
                         {
                                 int ind_i=ti*MTILE+ii*CTILE_ROWS+iii;
                                 int ind_j=tj*MTILE+jj*CTILE_COLS+jjj;
                                 TYPE_T c = read_channel_intel(CHANNEL_MATRIX_OUT);
                                 if(ind_i < N && ind_j<M)
                                     C[ind_i*ldc+ind_j]=beta*C[ind_i*ldc+ind_j]+c;
                         }
                 }
        }
    }


}
