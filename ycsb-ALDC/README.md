# YCSB-C

Yahoo! Cloud Serving Benchmark in C++, a C++ version of YCSB (https://github.com/brianfrankcooper/YCSB/wiki)

## Quick Start

To build YCSB-C on Ubuntu, for example:

```
$ sudo apt-get install libtbb-dev
$ make
```

As the driver for Redis is linked by default, change the runtime library path
to include the hiredis library by:
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

Run Workload A with a [TBB](https://www.threadingbuildingblocks.org)-based
implementation of the database, for example:
```
./ycsbc -db tbb_rand -threads 4 -P workloads/workloada.spec
```
Also reference run.sh and run\_redis.sh for the command line. See help by
invoking `./ycsbc` without any arguments.

Note that we do not have load and run commands as the original YCSB. Specify
how many records to load by the recordcount property. Reference properties
files in the workloads dir.

*AiKV
./ycsbc -db leveldb  -P workloads/workloada.spec -db_path /mnt/ssd/ldb -write_buffer_size 8 -block_size 4 -max_open_files 1000 -L0_CompactionTrigger 4 -L0_SlowdownWritesTrigger 8 -L0_StopWritesTrigger 12 -LSMFanout 10 -LDCStartLevel 1 -LDCEndLevel 7 -UseAdaptiveLDC false -LDCMergeSizeRatio 0.5 -max_file_size 4 -ThresholdBufferNum 10 -LDCBCCProbeInterval 5

-P means single workload to use, you can use -S adcdef for hybrid workload

-db_path means leveldb path, default at /mnt/ssd/ldb

-write_buffer_size means Memtable's size, default 4MB

-block_size  means all block's size, default 4KB

-max_file_size means SSTable's size,default 2M, half of Memtable's size normally

-L0_CompactionTrigger means Level0's compaction trigger threshold value, default 4, may affects performance

-L0_SlowdownWritesTrigger means leveldb may use sleep() to slow down Put() operation, may affects performance 

-L0_StopWritesTrigger means totally stop the Put() operation

-LSMFanout means ratio = lower_level size / uper_level_size, can determine the LSMtree's shape that fat or slim, it also can affect the LSMtree's total level numbers

-max_open_files means os open files limit 
#cyf add at 4.12 for LDC
-LDCStartLevel means Low-level Driven Compaction's start level, normally from 1 to 7, like [LDCStartLevel,LDCEndLevel].

-LDCEndLevel means Low-level Driven Compaction's end level, could set from 1 to 7, while end >start level [LDCStartLevel,LDCEndLevel].

-LDCMergeSizeRatio means Compaction trigger for LDC’s threshold，normally between 0.1 ~ 2.0 float type.

-UseAdaptiveLDC use ALDC if it is true.

-ThresholdBufferNum LDC's compaction trigger threshold, 1~10 int type

-LDCBCCProbeInterval probe period interval to sample the throughput and some compaction detail，default 5 second.

output.txt at the same dir with ycsbc to show the leveldb's state info
