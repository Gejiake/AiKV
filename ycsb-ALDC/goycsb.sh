sudo rm -rf /mnt/ssd/ldb
sudo rm -f output.txt
./ycsbc -db leveldb  -P workloads/workloada.spec -db_path /mnt/ssd/ldb -write_buffer_size 8 -block_size 4 -max_open_files 100000 -L0_CompactionTrigger 4 -L0_SlowdownWritesTrigger 8 -L0_StopWritesTrigger 12 -LSMFanout 10 -LDCStartLevel 7 -LDCEndLevel 7 -UseAdaptiveLDC false -LDCMergeSizeRatio 0.5 -max_file_size 4 -ThresholdBufferNum 10 -LDCBCCProbeInterval 5
