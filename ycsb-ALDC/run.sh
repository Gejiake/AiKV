#!/usr/bin/env bash

#Loop rounds
repeat_num=1
db_names=(
  "leveldb"
  #"lock_stl"
  #"tbb_rand"
  #"tbb_scan"
)


write_buffer_size=$1
block_size=$2
max_open_files=$3
max_file_size=$4


trap 'kill $(jobs -p)' SIGINT

#if [ $# -ne 1 ]; then
#  echo "Usage: $0 [dir of workload specs]"
#  exit 1
#fi

# workload_dir=$1
workload_dir=workloads

for file_name in $workload_dir/workloada.spec; do
  for ((tn=1; tn<=1; tn=tn+1)); do
    for db_name in ${db_names[@]}; do
      for ((i=1; i<=repeat_num; ++i)); do
        echo "Running $db_name with $tn threads for $file_name"
	cd /media/gejiake/application/YCSB-C-master
        ./ycsbc -db $db_name -threads $tn -P $file_name -write_buffer_size $write_buffer_size -block_size $block_size -max_open_files $max_open_files -max_file_size $max_file_size 2>>ycsbc.output &
        wait
      done
    done
  done
done
