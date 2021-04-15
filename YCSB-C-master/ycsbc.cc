//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <future>
#include "core/utils.h"
#include "core/timer.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "db/db_factory.h"
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <ctime>//cyf add for judge LOAD and RUN stages' starting time
//cyf add the value should not be modified
#define LONG_TAIL_LATENCY 154
#define ABS_WOKLOAD_PATH "./workloads"
#define WORKLOAD_MAX_NUM 10

//cyf copy from leveldb for stats tail latency
static const double BucketLimit[LONG_TAIL_LATENCY] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 25, 30, 35, 40, 45,
  50, 60, 70, 80, 90, 100, 120, 140, 160, 180, 200, 250, 300, 350, 400, 450,
  500, 600, 700, 800, 900, 1000, 1200, 1400, 1600, 1800, 2000, 2500, 3000,
  3500, 4000, 4500, 5000, 6000, 7000, 8000, 9000, 10000, 12000, 14000,
  16000, 18000, 20000, 25000, 30000, 35000, 40000, 45000, 50000, 60000,
  70000, 80000, 90000, 100000, 120000, 140000, 160000, 180000, 200000,
  250000, 300000, 350000, 400000, 450000, 500000, 600000, 700000, 800000,
  900000, 1000000, 1200000, 1400000, 1600000, 1800000, 2000000, 2500000,
  3000000, 3500000, 4000000, 4500000, 5000000, 6000000, 7000000, 8000000,
  9000000, 10000000, 12000000, 14000000, 16000000, 18000000, 20000000,
  25000000, 30000000, 35000000, 40000000, 45000000, 50000000, 60000000,
  70000000, 80000000, 90000000, 100000000, 120000000, 140000000, 160000000,
  180000000, 200000000, 250000000, 300000000, 350000000, 400000000,
  450000000, 500000000, 600000000, 700000000, 800000000, 900000000,
  1000000000, 1200000000, 1400000000, 1600000000, 1800000000, 2000000000,
  2500000000.0, 3000000000.0, 3500000000.0, 4000000000.0, 4500000000.0,
  5000000000.0, 6000000000.0, 7000000000.0, 8000000000.0, 9000000000.0,
  1e200,
};

//cyf add mutex concurrent stats
static std::mutex ld_mtx;
static std::mutex run_mtx;
//static std::array<double,LONG_TAIL_LATENCY> load_latency = {0.0};
//static std::array<double,LONG_TAIL_LATENCY> run_latency = {0.0};
static double load_latency[LONG_TAIL_LATENCY] = {0};
static double run_latency[LONG_TAIL_LATENCY] = {0};
//cyf add add some ugly stats info
static double total_loadtime = 0;
static double total_runtime = 0;

static double total_loadnum = 0;
static double total_runnum = 0;

static double max_loadlatency = 0;
static double max_runlatency = 0;
static double min_loadlatency = 9999.0;
static double min_runlatency = 9999.0;



using namespace std;

void PrintPercentLatency(double* array, double percent)
{
    //double p = percent;
    double opcounter = 0;
    double partialLatency = 0;
    double tail_num = 0;

    for(int i=0; i<LONG_TAIL_LATENCY;i++)
    {
        if(array[i] <= 0) continue;
        opcounter = opcounter + array[i];
        if((opcounter / total_runnum) >= percent){
            partialLatency = partialLatency + BucketLimit[i] * array[i];
            tail_num = tail_num + array[i];

        }

    }
    std::cout<<"----------------------------------------------------------------"<<std::endl;
    std::cout<<"PrintPercentLatency partialLatency: "<<partialLatency<<std::endl;
    std::cout<<"PrintPercentLatency tail_num: "<<tail_num<<std::endl;
    std::cout<<"PrintPercentLatency total_runnum: "<<total_runnum<<std::endl;
    std::cout<<"The Percent "<<100*percent<<"% 's latency is: "<<partialLatency/tail_num<<" us"<<std::endl;
    //print to txt
    std::ofstream out("./output.txt",ios::app);
    out <<100*percent <<"%: " << partialLatency/tail_num <<" ";
    out.close();
}

//cyf add to get the ops' latency info
void PrintHistogram()
{
    std::cout<<"The Histogram Load Stage:\t"<<std::endl;
    std::cout<<"The max load latency:\t"<<max_loadlatency<<std::endl;
    std::cout<<"The min load latency:\t"<<min_loadlatency<<std::endl;
    std::cout<<"The average load latency:\t"<< total_loadtime / total_loadnum<<"  us"<<std::endl;
//    PrintPercentLatency(load_latency, 0.50);
//    PrintPercentLatency(load_latency, 0.90);
//    PrintPercentLatency(load_latency, 0.95);
//    PrintPercentLatency(load_latency, 0.99);
//    PrintPercentLatency(load_latency, 0.999);
//    PrintPercentLatency(load_latency, 0.9999);

    std::cout<<"======================================================================"<<std::endl;

    std::cout<<"The Histogram Run Stage:\t"<<std::endl;
    std::cout<<"The max Run latency:\t"<<max_runlatency<<std::endl;
    std::cout<<"The min Run latency:\t"<<min_runlatency<<std::endl;
    std::cout<<"The average Run latency:\t"<< total_runtime / total_runnum<<"  us"<<std::endl;
//    PrintPercentLatency(run_latency, 0.50);
//    PrintPercentLatency(run_latency, 0.90);
//    PrintPercentLatency(run_latency, 0.95);
    PrintPercentLatency(run_latency, 0.99);
//    PrintPercentLatency(run_latency, 0.999);
//    PrintPercentLatency(run_latency, 0.9999);
    //print to txt 'A newline'
    std::ofstream out("./output.txt",ios::app);
    out <<std::endl;
    out.close();
}

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], std::vector<utils::Properties> &props);

//cyf add latency info into latency array
void AddLatencyToArray(double latency, double* array,int current_ops)
{
    //assert(current_ops > 0);
    double v = latency / current_ops;
    int b = 0;
    while(b < LONG_TAIL_LATENCY-1 && BucketLimit[b]<= v){
        b++;
    }
    array[b] += 1.0;

}

int DelegateClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const int num_ops,
    bool is_loading) {
  db->Init();
  ycsbc::Client client(*db, *wl);
  utils::Timer_us timer_us;
  //utils::Timer<double> timer_us;
  double duration;

  double sum_time = 0;
  double sum_num = 0;
  double max_latency = 0;
  double min_latency = 9999.0;
  int current_ops;

  double array[LONG_TAIL_LATENCY] = {0};
  int oks = 0;
  for (int i = 0; i < num_ops; ++i) {
      duration = 0;
      current_ops = 0;
      timer_us.Start();
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
      //current_ops = client.DoTransaction();
      //oks +=current_ops;
    }
     duration = timer_us.End();
     if(min_latency > duration) min_latency = duration;
     if(max_latency < duration) max_latency = duration;
     sum_time += duration;
     //sum_num += oks;
     //AddLatencyToArray(duration,array,current_ops);
     AddLatencyToArray(duration,array,1);
  }
    sum_num = (double)oks;
  if(is_loading){
      ld_mtx.lock();

      if(min_loadlatency > min_latency) min_loadlatency = min_latency;
      if(max_loadlatency < max_latency) max_loadlatency = max_latency;
      total_loadtime += sum_time;
      total_loadnum += sum_num;
      for(int i=0;i<LONG_TAIL_LATENCY;i++)
          load_latency[i] = load_latency[i] + array[i];

      ld_mtx.unlock();
  }else{
      run_mtx.lock();
      if(min_runlatency > min_latency) min_runlatency = min_latency;
      if(max_runlatency < max_latency) max_runlatency = max_latency;
      total_runtime += sum_time;
      total_runnum += sum_num;
      for(int i=0;i<LONG_TAIL_LATENCY;i++) run_latency[i] = run_latency[i] + array[i];
      run_mtx.unlock();
  }
  db->Close();
  return oks;
}


int main(const int argc, const char *argv[]) {
  utils::Properties proper_first;
  std::vector<utils::Properties> props_set;
  props_set.push_back(proper_first);
  string file_name = ParseCommandLine(argc, argv, props_set);
  //uint64_t TOTAL_OPS_INCLUDE_LOAD_AND_RUN = 0;
  //double   TOTAL_TIME_INCLUDE_LOAD_AND_RUN = 0;

  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props_set[0]);
  if (!db) {
    cout << "Unknown database name " << props_set[0]["dbname"] << endl;
    exit(0);
  }

  //std::vector<ycsbc::CoreWorkload> wl_set;
  ycsbc::CoreWorkload wl_set[WORKLOAD_MAX_NUM];
  for (size_t i = 0;i< props_set.size(); i++) {
      //ycsbc::CoreWorkload wl;
      wl_set[i].Init(props_set[i]);
      //wl_set.push_back(wl);

  }


  const int num_threads = stoi(props_set[0].GetProperty("threadcount", "1"));

    // Loads data
    vector<future<int>> actual_ops;
    int total_ops = stoi(props_set[0][ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
    utils::Timer<double> load_timer;

    memset(load_latency, 0 ,sizeof(double) * LONG_TAIL_LATENCY);
    memset(run_latency, 0 ,sizeof(double) * LONG_TAIL_LATENCY);

    time_t load_now = time(0);
    std::string load_time = ctime(&load_now);
    std::cout << "Load stage start time: "<<load_time<< std::endl;
    load_timer.Start();
    for (int i = 0; i < num_threads; ++i) {
      actual_ops.emplace_back(async(launch::async,
          DelegateClient, db, &wl_set[0], total_ops / num_threads, true));
      std::cout<<"The total load insertion num: "<<total_ops<<std::endl;
    }
    assert((int)actual_ops.size() == num_threads);

    int sum = 0;
    for (auto &n : actual_ops) {
      assert(n.valid());
      sum += n.get();
    }
  double load_duration = load_timer.End();
  std::cout << "# Loading records:\t" << sum << endl;
  std::cout << "# Transaction throughput (KTPS)" << endl;
  std::cout <<"Load_stage: "<< props_set[0]["dbname"] << '\t' << file_name << '\t' << num_threads << '\t';
  std::cout << total_ops / load_duration / 1000 << endl;



    sleep(120);//wait 10second for background compaction finished

  if(!wl_set[0].isOnlyLoadStage())
  {
      std::cout<<"Not Only Load Stage, starting YCSB Run stage!"<<std::endl;
      time_t run_now = time(0);
      string run_time = ctime(&run_now);
      std::cout << "Load stage start time: "<<run_time<< std::endl;
      actual_ops.clear();
      total_ops = stoi(props_set[0][ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
      utils::Timer<double> timer;
      timer.Start();
      for(size_t j=0; j<props_set.size(); j++){
          assert(props_set.size() > 0);
            for (int i = 0; i < num_threads; ++i) {
                actual_ops.emplace_back(async(launch::async,
                    DelegateClient, db, &wl_set[j], total_ops / num_threads, false));
            }
            assert((int)actual_ops.size() == num_threads);

            sum = 0;
            for (auto &n : actual_ops) {
                assert(n.valid());
                sum += n.get();
            }
            actual_ops.clear();
      }
      double duration = timer.End();


      std::cout << "# Transaction throughput (KTPS)" << endl;
      std::cout <<"Run_stage: "<< props_set[0]["dbname"] << '\t' << file_name << '\t' << num_threads << '\t'
                << (props_set.size() * total_ops) / (duration * 1000) <<std::endl;

      std::cout << "# Transaction average throughput(Load+Run) (KTPS)"
                << '\t' << file_name << '\t' << num_threads << '\t'
                <<((props_set.size() +1 )* total_ops) / ((duration+load_duration) * 1000)
                << endl;

      //print to txt
      std::ofstream out("./output.txt",ios::app);
      out << "Throughput(KTPS): "<< (props_set.size() * total_ops) / (duration * 1000) <<std::endl;
      out <<"workload_read_ops: "<< ycsbc::wl_read_ops<<std::endl;
      out <<"workload_update_ops: "<< ycsbc::wl_update_ops<<std::endl;
      out <<"workload_insert_ops: "<< ycsbc::wl_insert_ops<<std::endl;
      out <<"workload_scan_ops: "<< ycsbc::wl_scan_ops<<std::endl;
      out <<"workload_readmodifywrite_ops: "<< ycsbc::wl_rmw_ops<<std::endl;
      out.close();



  }


  PrintHistogram();
  std::cout<<"total_loadnum:"<<total_loadnum<<" total_loadtime:"<<total_loadtime
          <<" total_runnum:"<<total_runnum<<" total_runtime:"<<total_runtime<<std::endl;


  std::cout<<"============================Run Latency Statistic========================\t"<<file_name<<std::endl;


  delete db;//cyf release memory space

}

std::string ParseCommandLine(int argc, const char *argv[], std::vector<utils::Properties> &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props[0].SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props[0].SetProperty("dbname", argv[argindex]);
      argindex++;
    }else if (strcmp(argv[argindex], "-db_path") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props[0].SetProperty("db_path", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-write_buffer_size") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("write_buffer_size", argv[argindex]);
        argindex++;
    }else if (strcmp(argv[argindex], "-LSMFanout") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("LSMFanout", argv[argindex]);
        argindex++;
    }else if (strcmp(argv[argindex], "-L0_CompactionTrigger") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("L0_CompactionTrigger", argv[argindex]);
        argindex++;
    }else if (strcmp(argv[argindex], "-L0_SlowdownWritesTrigger") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("L0_SlowdownWritesTrigger", argv[argindex]);
        argindex++;
    }else if (strcmp(argv[argindex], "-L0_StopWritesTrigger") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("L0_StopWritesTrigger", argv[argindex]);
        argindex++;
    }else if (strcmp(argv[argindex], "-block_size") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("block_size", argv[argindex]);
        argindex++;
    } else if (strcmp(argv[argindex], "-max_open_files") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("max_open_files", argv[argindex]);
        argindex++;
    } else if (strcmp(argv[argindex], "-max_file_size") == 0) {
        argindex++;
        if (argindex >= argc) {
            UsageMessage(argv[0]);
            exit(0);
        }
        props[0].SetProperty("max_file_size", argv[argindex]);
        argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props[0].Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else if(strcmp(argv[argindex], "-S") == 0){
      //cyf add for hybrid workloads like a,c,b,d,f,e and so on.
      argindex++;
      std::string wls = argv[argindex];
      std::string abs_wl_path = ABS_WOKLOAD_PATH;    
      ifstream input_0(abs_wl_path + wls[0] + ".spec");
      try {
        std::cout << "the hybrid workloads: " << abs_wl_path + wls[0] + ".spec" <<std::endl;
        props[0].Load(input_0);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input_0.close();

      for (size_t i = 1; i < wls.size(); i++) {
          utils::Properties p;
          ifstream in(abs_wl_path + wls[i] + ".spec");
          try {
            std::cout << "the hybrid workloads: " << abs_wl_path + wls[i] + ".spec" <<std::endl;
            p.Load(in);
          } catch (const string &message) {
            cout << message << endl;
            exit(0);
          }
          in.close();
          props.push_back(p);
      }//end for (size_t i = 1;i < wls.size(); i++)

      filename = abs_wl_path + argv[argindex] + ".spec";
      argindex++;

    }else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
  cout << "                   be specified, and will be processed in the order specified" << endl;
  cout << "  -S hybrid propertyfiles: l '-S abcdef' , means run workload a-f sequentially" << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

