//
//  basic_db.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/17/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include "db/db_factory.h"

#include <string>

//#include "db/rocksdb_db.h"
#include "db/leveldb_db.h"
//#include "db/pmemkv_db.h"

using namespace std;
using ycsbc::DB;
using ycsbc::DBFactory;

DB* DBFactory::CreateDB(utils::Properties &props) {

//add rocksDB here soon.	
    if(props["dbname"]== "rocksdb") {
		//return new RocksDB;
    }
  //add LevelDB here soon.
    else if(props["dbname"]== "leveldb") {

        uint64_t write_buffer_size = stoi(props.GetProperty("write_buffer_size","4"));//4MB
        uint64_t block_size = stoi(props.GetProperty("block_size","4"));//4KB
        int max_open_files = stoi(props.GetProperty("max_open_files","1000"));
        uint64_t max_file_size = stoi(props.GetProperty("max_file_size","2"));//2MB
        //cyf add for auto-tuning
        int L0_CompactionTrigger = stoi(props.GetProperty("L0_CompactionTrigger","4"));
        int L0_SlowdownWritesTrigger = stoi(props.GetProperty("L0_SlowdownWritesTrigger","8"));
        int L0_StopWritesTrigger  = stoi(props.GetProperty("L0_StopWritesTrigger","12"));
        int LSMFanout = stoi(props.GetProperty("LSMFanout","10"));
        std::string db_path =props.GetProperty("db_path","/mnt/ssd/ldb"); 

        int LDCStartLevel = stoi(props.GetProperty("LDCStartLevel","1"));
        int LDCEndLevel = stoi(props.GetProperty("LDCEndLevel","7"));
        double LDCMergeSizeRatio = stod(props.GetProperty("LDCMergeSizeRatio","0.5"));
        std::string UseAdaptiveLDC = props.GetProperty("UseAdaptiveLDC","false");

        bool UALDC = false;
        if(UseAdaptiveLDC == "true") UALDC = true;

        int ThresholdBufferNum = stoi(props.GetProperty("ThresholdBufferNum","10"));

        return new Level_DB(
         db_path,
         write_buffer_size<<20,
         block_size<<10, 
         max_open_files,
         max_file_size<<20,
         L0_CompactionTrigger,
         L0_SlowdownWritesTrigger,
         L0_StopWritesTrigger,
         LSMFanout,
         LDCStartLevel,
         LDCEndLevel,
         LDCMergeSizeRatio,
         UALDC,
         ThresholdBufferNum);
    }

	else return NULL;
}

