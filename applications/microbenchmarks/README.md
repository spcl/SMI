## Bandwidth

## Latency

For the latency we measured it by using a ping/pon benchmark.
The 4 FPGAs are organized in a bus so we can test at different hop distance

## Ingestion rate

We have two rank:
- on rank 0 app_0 sends messages of size 1 to app_1 (tag 0) and app_2 (tag 1)
- on rank 1 app_1/app_2 receive

The purpose is to see what is the ingestion rate.
Potential bottleneck will be the CK_S/CK_R


What we can do is to show how the ingestion rate changes with the number of QSFP used
the fact that we have two sends here is useless.
In the code there is a macro (SINGLE_QSFP) and also we disable the second application

## Broadcast

A message composed by N floats is streamed to all the other ranks
