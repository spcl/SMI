//Attempt to re-create the broadcast bug

//we have a computation that receives data from a second kernel (currently it does nothing)

#pragma OPENCL EXTENSION cl_intel_channels : enable

typedef struct{
	bool request;
	int data;
}message_t;

typedef struct{
	bool start;
	message_t m;
}computation_t;

channel message_t channels[2] __attribute__((depth(2)));

void receive(computation_t *status, int *data)
{
	
	if(status->start){
		//send the request for data
		status->m.request=true;
		status->m.data=1000;
		write_channel_intel(channels[0],status->m);
		status->start=false;
	}
	//receive the data, increment and store in memory
	status->m=read_channel_intel(channels[1]);
	*data=status->m.data;
}
__kernel void app(const int N, const int start, __global int *mem)
{
	message_t m;
	int data;
	computation_t status;
	status.start=true;
	for(int i=0;i<N;i++)
	{
		receive(&status,&data);
		
		mem[i]=data;

	}



}


__kernel void generator()
{
	//receive the request
	message_t m=read_channel_intel(channels[0]);
	for(int i=0;i<m.data;i++)
	{
		message_t send;
		send.data=i;
		send.request=false;
		write_channel_intel(channels[1],send);
	}
}