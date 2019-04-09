//Bandwdith for host based Bandwdith
//In this case: 
//- the host copy to the device the data arriving from the remote
//- the application reads the data from memory 
//notice that the fact that we are using a single memory module is not a problem since it is clearly faster
//than the network
#pragma OPENCL EXTENSION cl_khr_fp64 : enable


//N is the total amount of data generated in memory
__kernel void app_0(__global double* memory,const int N)
{
	const int loop_limit=N/3;
	 double maxv=0;
    for(int i=0;i<loop_limit;i++)
    {
    	double data[3];
        data[0]=memory[i*3];
        data[1]=memory[i*3+1];
        data[2]=memory[i*3+2];
        if(data[0]>maxv) maxv=data[0];
        if(data[1]>maxv) maxv=data[1]; 
        if(data[2]>maxv) maxv=data[2]; 
    }
    memory[0]=maxv;

}