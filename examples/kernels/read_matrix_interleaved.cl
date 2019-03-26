/**
	The kernels read the matrix, which is interleaved
	among different modules

	Elements are interleaved with size 16 float elements (64 bytes)
	We impose that M is a multiple of 64, so we know that a row starts
	always from the first module

*/


#pragma OPENCL EXTENSION cl_intel_channels : enable
#define W 64

channel float channel_matrix_A __attribute__((depth(W)));


__kernel void read_matrix_A(__global volatile  float *restrict data0,__global volatile  float *restrict data1,
		__global volatile  float *restrict data2, __global volatile  float *restrict data3, int N, int M, unsigned int lda)
{
    const int loop_it=((int)(M))/64;   //W must be a divisor of M
    const int multiply64=1+(int)((lda-1)/64); //lda must be a multiple of 64, otherwise inefficient hw is generated for the load

    float to_send[W];
	for(int i=0;i<N;i++)
	{
		for(int j=0;j<loop_it;j++)
		{
			//load from memory
			#pragma unroll
			for(int k=0;k<16;k++)
			{
				to_send[k]=data0[i*16*multiply64+j*16+k];
			}
			//if(i<2)
			//	printf("Primo elemento letto da A_0 %.1f\n",to_send[0]);

			#pragma unroll
			for(int k=0;k<16;k++)
			{
				to_send[k+16]=data1[i*16*multiply64+j*16+k];

			}
			//if(i<2)
			//	printf("Primo elemento letto da A_1 %.1f\n",to_send[16]);

			#pragma unroll
			for(int k=0;k<16;k++)
			{
				to_send[k+32]=data2[i*16*multiply64+j*16+k];

			}
			//if(i<2)
			//	printf("Primo elemento letto da A_2 %.1f\n",to_send[32]);

			#pragma unroll
			for(int k=0;k<16;k++)
			{
				to_send[k+48]=data3[i*16*multiply64+j*16+k];

			}
			//if(i<2)
			//	printf("Primo elemento letto da A_3 %.1f\n",to_send[48]);

			#pragma unroll
            for(int k = 0; k < W; k++)
            {
                write_channel_intel(channel_matrix_A,to_send[k]);
            }

		}	
	}
}



__kernel void receiver(__global volatile  float *restrict mem, int N, int M)
{
    const int loop_it=((int)(M))/W;   //W must be a divisor of M
    float  sum=0;
    float recv[W];
    //float expected=0.0f;
    //int errors=0;
	for(int i=0;i<N;i++)
	{
		for(int j=0;j<loop_it;j++)
		{
			float acc=0;
			#pragma unroll
            for(int k = 0; k < W; k++)
            {
            	recv[k]=read_channel_intel(channel_matrix_A);
        		acc+=recv[k];
        		/*if(recv[k]!=expected && errors <16)
        		{
        			printf("Error on element %d, %d (%.1f != %.1f) \n",i,j*W+k,recv[k],expected);
        			errors++;
        		}
        		expected++;*/
    		}
    		sum+=acc;
		}	
	}
    *mem=sum;
}