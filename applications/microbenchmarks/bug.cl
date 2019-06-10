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
	message_t m2;

}computation_t;

channel message_t channels[3] __attribute__((depth(2)));

void receive(computation_t *status, bool first_generator, int *data)
{

	if(first_generator)
	{

		write_channel_intel(channels[2],status->m);

	}
	else
	{
		if(status->start){
			//send the request for data
			status->m2.request=true;
			status->m2.data=1000;
			status->start=false;
				write_channel_intel(channels[0],status->m2);
		}
		//receive the data, increment and store in memory
		status->m=read_channel_intel(channels[1]);
		*data=status->m2.data;
	}

}
__kernel void app(const int N, const int start, char gen, __global int *mem)
{
	message_t m;
	int data;
	computation_t status;
	status.start=true;
	bool first=(gen==0);
	for(int i=0;i<N;i++)
	{
		receive(&status,first,&data);
		data++;
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



__kernel void receiver()
{
	while(true)
	{

		message_t m=read_channel_intel(channels[2]);
	}
}