//
//  basic_db.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/17/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include "db/db_factory.h"

#include <string>
#include "db/basic_db.h"
#include "db/lock_stl_db.h"
#include "db/redis_db.h"
#include "db/tbb_rand_db.h"
#include "db/tbb_scan_db.h"
#include "db/rocksdb_db.h"
#include "db/leveldb_db.h"
//#include "db/pmemkv_db.h"

using namespace std;
using ycsbc::DB;
using ycsbc::DBFactory;

DB* DBFactory::CreateDB(utils::Properties &props) {
  if (props["dbname"] == "basic") {
    return new BasicDB;
  } 
	else if (props["dbname"] == "lock_stl") {
    return new LockStlDB;
  } 
	else if (props["dbname"] == "redis") {
    int port = stoi(props["port"]);
    int slaves = stoi(props["slaves"]);
    return new RedisDB(props["host"].c_str(), port, slaves);
  } 
	else if (props["dbname"] == "tbb_rand") {
    return new TbbRandDB;
  } 
	else if (props["dbname"] == "tbb_scan") {
    return new TbbScanDB;
  } 
//add rocksDB here soon.	
	else if(props["dbname"]== "rocksdb") {
		return new RocksDB;
  }
  //add LevelDB here soon.
    else if(props["dbname"]== "leveldb") {
        int write_buffer_size = stoi(props["write_buffer_size"]);
        int block_size = stoi(props["block_size"]);
        int max_open_files = stoi(props["max_open_files"]);
        int max_file_size = stoi(props["max_file_size"]);
        return new Level_DB(write_buffer_size, block_size, max_open_files, max_file_size);
    }
//add pmemkv here
    /*else if(props["dbname"]=="pmemkv"){
        return new PmemKV;
  }*/
	else return NULL;
}

