I've done the "Ingestion rate" benchmark: the purpose of this microbench. is to see at which frequency SMI is able to serve two consecutive sends (or how many cycles are needed to SMI to let you do another communication).

The test is composed by two ranks. Rank 0 in which an application send to two applications and Rank 1 in which the two applications receive. The code of the application on rank 0 is:

```

__kernel void app_0(const int N)
{
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send1=SMI_Open_send_channel(1,SMI_INT,1,0);
        SMI_Push(&chan_send1,&i);
        SMI_Channel chan_send2=SMI_Open_send_channel(1,SMI_INT,1,1);
        SMI_Push(&chan_send2,&i);
    }
}
```
So, at each loop iteration it opens two channeld (message of size one) and sends. 

In the ideal situation, if everything tooks one cycle, the loop is fully pipelined and the execution time should be ~N/freq.

Now, I've taken N=1Billion, and given the running frequency (340Mhz), I was expecting a running time of **2.9** seconds. I've executed the two rank in two FPGAs connected by two cables. At the beginning I was using only one cable and the execution time was **29** seconds, so **10x slower** than the ideal.
The reason is always the same. In this case the receiving CK_R (the one attached to the cable) slow downs everything. Since it reads from the I/O **once every 5 cycles** (it reads from I/O, 1 attached CK_S and 3 CK_R) and it was receiving **2B** messages (we are sending to two application), we get that **10X** slow down.

As an additional test, I've exploited also the other connection cable. **Thanks to our routing** this was really simple, just a change in the host code, no need to recompile. 
In this case the execution time was **14.5** seconds, so **5x** slower, which is inline with the CK_R reading once every 5 clock cycles (but also the CK_S has similar behavior).

Now:

- we can reprogram the benchmark and use only one QSFP. This will save us from the bottleneck
- we can leave it as it is, but it should be clear that this is the injestion rate **from the same application endpoint**. In any case, CK_S/CK_R are able to serve 1 packet per clock cycle.
