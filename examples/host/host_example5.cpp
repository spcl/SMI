/**
 	Example 5 is a MPMD program, characterized by three ranks:
    - rank 0: has two application kernels App_0 and App_1
    - rank 1: has one application kernel App_2
    - rank 3: has one application kernel App_3

    The logic is the following:
    - App_0 sends N integers to App_2 through a proper channel (TAG 0)
    - App_1 sends N floats to App_3 through a proper channel (TAG 1)

	The program will be execute on three FPGA nodes ( node 15 acl0, 15:1 and 16:1)
	using the following connections:
	- 15:0:chan1 - 15:1:chan1
	- 15:1:chan0 - 16:1:chan0

	So we don t use the crossbar. This is done to see if everything works with an intermediate hop

	Also, considering that for the moment being we use only 2 QSFPs and numbering the CK_S with 0 and 1
	we impose that:
	- APP 0 is connected to CK_S_0: in this way its sent data must go to CK_S_1 and then to QSFP
	- APP 1 is connected to CK_S_1: so data will go through QSFP1, will be received to CK_R_1 on rank 1
		which will route it to its paired CK_S_1, who will route it to CK_S_1 (in rank 1) who finally
		sends it to rank 2
	- APP_2 is connected to CK_R_1 (so no additional hop)
	- APP_3 is connected to CK_R_0 (so no additional hop)

	Therefore the routing table for CK_Ss will be: (remember 0-> QSFP, 1 CK_R, 2... connected CK_S. This maps ranks-> ids)
	- rank 0: 	CK_S_0: 1,2,2
				CK_S_1: 1,0,0
	- rank 1:	CK_S_0: 2,1,0
				CK_S_1: 0,1,2
	- rank 2: meaningless

	The routing tables for CK_Rs, instead, maps TAGs to internal port id (0,...Q-2 other CK_R, others: application endpoints):
	- rank 0: meaningless
	- rank 1: 	CK_R_0: meaningless (no attached endpoints), if it receives something with TAG 0 it should send to CK_R_1 but we are not using it
				CK_R_1: 2,xxx (send to APP_2...tag 1 is already handled by looking at the rank and forwarding to the CKS)
	- rank 2: 	CK_R_0: xxx, 2


 */



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"

//#define CHECK
using namespace std;
int main(int argc, char *argv[])
{

    //command line argument parsing
    if(argc<9)
    {
        cerr << "Send/Receiver tester " <<endl;
        cerr << "Usage: "<< argv[0]<<" -b <binary file> -n <length> -r <rank 0/1/2> -f <fpga> "<<endl;
        exit(-1);
    }
    int n;
    int c;
    std::string program_path;
    char rank;
    int fpga;
    while ((c = getopt (argc, argv, "n:b:r:f:")) != -1)
        switch (c)
        {
            case 'n':
                n=atoi(optarg);
                break;
            case 'b':
                program_path=std::string(optarg);
                break;
            case 'f':
                fpga=atoi(optarg);
                break;
            case 'r':
                {
                    rank=atoi(optarg);
                    if(rank!=0 && rank!=1 && rank!=2)
                    {
                        cerr << "Error: rank may be 0-2"<<endl;
                        exit(-1);
                    }

                    break;
                }

            default:
                cerr << "Usage: "<< argv[0]<<"-b <binary file> -n <length>"<<endl;
                exit(-1);
        }

    cout << "Performing send/receive test with "<<n<<" integers"<<endl;

    cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names;

    switch(rank)
    {
        case 0:
            cout << "Starting rank 0 on FPGA "<<fpga<<endl;
            kernel_names.push_back("app_0");
            kernel_names.push_back("app_1");
            kernel_names.push_back("CK_S_0");
            kernel_names.push_back("CK_S_1");
            kernel_names.push_back("CK_R_0");
            kernel_names.push_back("CK_R_1");
            break;
        case 1:
        	cout << "Starting rank 1 on FPGA "<<fpga<<endl;
        	kernel_names.push_back("app_2");
                kernel_names.push_back("CK_S_0");
                kernel_names.push_back("CK_S_1");
                kernel_names.push_back("CK_R_0");
                kernel_names.push_back("CK_R_1");

        break;

        case 2:
        	cout << "Starting rank 2 on FPGA "<<fpga<<endl;
        	kernel_names.push_back("app_3");
                kernel_names.push_back("CK_S_0");
                kernel_names.push_back("CK_S_1");
                kernel_names.push_back("CK_R_0");
                kernel_names.push_back("CK_R_1");

        break;

    }

    //this is for the case with classi channels
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,program_path,kernel_names, kernels,queues);

    //create memory buffers
    char res;
    //ATTENTION: declare all the memory here otherwise it hangs
    cl::Buffer check(context,CL_MEM_WRITE_ONLY,1);

        char ranks=3;

    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,ranks);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,2);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,2);
    if(rank==0)
    {
        //senders routing tables
        char rt_s_0[3]={1,2,2}; 
        char rt_s_1[3]={1,0,0}; 
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        //receivers routing tables: these are meaningless
        char rt_r_0[2]={100,100}; //the first doesn't do anything
        char rt_r_1[2]={100,100};   //the second is attached to the ck_r (channel has TAG 1)
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,2,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,2,rt_r_1);
        //args for the apps
        kernels[0].setArg(0,sizeof(int),&n);
        kernels[1].setArg(0,sizeof(int),&n);
        //args for the CK_Ss
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[2].setArg(1,sizeof(char),&ranks);
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[3].setArg(1,sizeof(char),&ranks);

        //args for the CK_Rs
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[4].setArg(1,sizeof(char),&rank);
        kernels[4].setArg(2,sizeof(char),&ranks);
        kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[5].setArg(1,sizeof(char),&rank);
        kernels[5].setArg(2,sizeof(char),&ranks);

    }
    if(rank==1)
    {

        char ranks=3;
        char rt_s_0[3]={2,1,0}; 
        char rt_s_1[3]={0,1,2}; 
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={4,100}; //the first  is connect to application endpoint (TAG=0)
        char rt_r_1[2]={1,100};
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,2,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,2,rt_r_1);

        //args for the apps
        kernels[0].setArg(0,sizeof(cl_mem),&check);
        kernels[0].setArg(1,sizeof(int),&n);

        //args for the CK_Ss
        kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[1].setArg(1,sizeof(char),&ranks);
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[2].setArg(1,sizeof(char),&ranks);

        //args for the CK_Rs
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[3].setArg(1,sizeof(char),&rank);
        kernels[3].setArg(2,sizeof(char),&ranks);
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[4].setArg(1,sizeof(char),&rank);
        kernels[4].setArg(2,sizeof(char),&ranks);

    }

    if(rank==2)
    {

        char ranks=3;
        //these are meaningless
        char rt_s_0[3]={1,1,1}; 
        char rt_s_1[3]={1,1,1}; 
        queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,ranks,rt_s_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,ranks,rt_s_1);
        //receivers routing tables
        char rt_r_0[2]={100,4}; //the first  is connect to application endpoint (TAG=0)
        char rt_r_1[2]={100,4};   //the second doesn't do anything
        queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,2,rt_r_0);
        queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,2,rt_r_1);

        //args for the apps
        kernels[0].setArg(0,sizeof(cl_mem),&check);
        kernels[0].setArg(1,sizeof(int),&n);

        //args for the CK_Ss
        kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
        kernels[1].setArg(1,sizeof(char),&ranks);
        kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
        kernels[2].setArg(1,sizeof(char),&ranks);

        //args for the CK_Rs
        kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
        kernels[3].setArg(1,sizeof(char),&rank);
        kernels[3].setArg(2,sizeof(char),&ranks);
        kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
        kernels[4].setArg(1,sizeof(char),&rank);
        kernels[4].setArg(2,sizeof(char),&ranks);

    }


    timestamp_t start=current_time_usecs();
    for(int i=0;i<kernel_names.size();i++)
        queues[i].enqueueTask(kernels[i]);

    queues[0].finish();
    if(rank==0)
        queues[1].finish();


    timestamp_t end=current_time_usecs();

    std::cout << "Sleeping a bit to ensure CK have finished their job"<<std::endl;
    sleep(1);
    if(rank==1)
    {
        queues[0].enqueueReadBuffer(check,CL_TRUE,0,1,&res);
        if(res==1)
            cout << "RANK 1 OK!"<<endl;
        else
        	cout << "RANK 1 ERROR!"<<endl;

        cout << "Time elapsed (usecs): "<<end-start<<endl;
    }
    if(rank==2)
    {
        queues[0].enqueueReadBuffer(check,CL_TRUE,0,1,&res);
        if(res==1)
            cout << "RANK 2 OK!"<<endl;
        else
        	cout << "RANK 2 ERROR!"<<endl;

        cout << "Time elapsed (usecs): "<<end-start<<endl;
    }

}
