/**
	Simple kernel for evaluating the memory bandwdith
	achieved in reading a matrix from memory.
	The matrix is sent to a second kernel that checks
	that everything is in order.

	In this case we load 256 floats (1024 bytes) per time from the memory.
	Apparantly this it the width of the burst (2048 bit * 4 modules)

*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#define W 64

channel float channel_matrix_A __attribute__((depth(W)));


__kernel void read_matrix_A(__global volatile  float *restrict data, int N, int M, unsigned int lda)
{
    const int loop_it=((int)(M))/256;   //W must be a divisor of M
    const int multiply64=1+(int)((lda-1)/64); //lda must be a multiple of 64, otherwise inefficient hw is generated for the load

    float to_send[256];
	for(int i=0;i<N;i++)
	{
		for(int j=0;j<loop_it;j++)
		{
			//load from memory
			for(int k=0;k<256;k++)
				to_send[k]=data[i*64*multiply64+j*256+k];
			
			//send
			#pragma unroll 1
			for(int kk=0;kk<4;kk++)
			{
				#pragma unroll
		        for(int k = 0; k < W; k++)
		        {
		            write_channel_intel(channel_matrix_A,to_send[kk*64+k]);
		        }
        	}

		}	
	}
}



__kernel void receiver(__global volatile  float *restrict mem, int N, int M)
{
    const int loop_it=((int)(M))/W;   //W must be a divisor of M
    float  sum=0;
    float recv[W];
    float expected=1.0f;
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
    		}
    		sum+=acc;
		}	
	}
    *mem=sum;
}