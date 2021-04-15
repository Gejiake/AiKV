// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

#include <stdio.h>
#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/slice.h"
#include "leveldb/table_builder.h"
#include "util/coding.h"
#include "util/logging.h"
#include <atomic>

namespace leveldb {


// Grouping of constants.  We may want to make some of these
// parameters set via options.
namespace config {
static const int kNumLevels = 7;

// Level-0 compaction is started when we hit this many files.
static  int kL0_CompactionTrigger = 4;

// Soft limit on number of level-0 files.  We slow down writes at this point.
static  int kL0_SlowdownWritesTrigger = 8;

// Maximum number of level-0 files.  We stop writes at this point.
static  int kL0_StopWritesTrigger = 12;

//cyf add for auto-tuning to change Fanout
static  int kLSMFanout = 10;

// Maximum level to which a new compacted memtable is pushed if it
// does not create overlap.  We try to push to level 2 to avoid the
// relatively expensive level 0=>1 compactions and to avoid some
// expensive manifest file operations.  We do not push all the way to
// the largest level since that can generate a lot of wasted disk
// space if the same key space is being repeatedly overwritten.
//whc change
static const int kMaxMemCompactLevel = 2;//cyf changed

// Approximate gap in bytes between samples of data read during iteration.
static const int kReadBytesPeriod = 1048576;

static const uint64_t kBloomFilterBitsPerKey = 10;

static const uint64_t kLDCBlockCacheSize = 8 << 20;//cyf change default 8MB

static uint64_t kLDCMaxFileSizeLimit = 4 << 20;
static uint64_t kLDCBlockSize = 4 << 10;
static uint64_t kLDCMaxWriteBufferSize = kLDCMaxFileSizeLimit * 2;
//static const bool kLDCGetOneLevelOnce = true;
//whc add
static  int kBufferCompactStartLevel  = 1;//need to check Start Level < End Level
static  int kBufferCompactEndLevel = 7;

static  double kLDCMergeSizeRatio = 0.5;//cyf change to non-const var
static  bool   kUseAdaptiveLDC = false;
//cuttleTrees'option
static const bool kUseCattleTreeMethods = false;
static const uint32_t kCuttleTreeAmplifyFactor = kLSMFanout;
static const uint64_t kCuttleTreeFirstLevelSize = 5 * kLDCMaxFileSizeLimit;

static  uint64_t kLDCBCCProbeInterval = 5;//cyf probe every 30s
//whc add
static  int kThresholdBufferNum  = 10;
//cyf LDC trigger condition
//cyf add for having two condition to determine the merge operation
//kLDCMergeSizeRatio = total_linked_fragement_size / target_merge_sstable_size
//the best is 1:1, means no write amplification

static const bool kIsLDCSizeTrigger = true;

//cyf add for compation IO limit factor's threshold
static const double kCompactionIOLimitFactorThreshold = 2.0;


//cyf add for pre-reserving buffer number
static const int kBufferResveredNum = 20;

//cyf add for get percent size 0%~100% 's key, SST max size ~ 2MB
//|0%   |10%    |......|90%     |100%   |
//|key0 |key1   |......|key9    |key10  |   key0 = smallest, key10 = largest
static const int kLDCLinkKVSizeInterval = 11;
static double const kInitialLDCMergeSizeRatio = kLDCMergeSizeRatio;//cyf use for reset Ratio value for YCSB Run stage
//whc add
static const bool kSwitchSSD = false;

//whc add
static const std::string kSSDPath = "";


}  // namespace config


//whc add
class InternalKey;
class BCJudge{
    public:
    static bool IsBufferCompactLevel(int level){
        return (level >= config::kBufferCompactStartLevel && level<=config::kBufferCompactEndLevel);
    }
};

//whc add
struct ReadStaticDelta{
     uint64_t mem_get;
     uint64_t level_get[config::kNumLevels];
     uint64_t table_get;
     uint64_t table_bloomfilter_miss;
     uint64_t table_readfile_miss;
     uint64_t table_cache_shoot;
     uint64_t data_block_read;
     uint64_t index_block_size;
     uint64_t block_cache_read;
     int open_num;
     int get_flag;

     int put_num;
     int get_num;

};

class ReadStatic{
  public:
    static uint64_t mem_get;
    static uint64_t level_get[config::kNumLevels];
    static uint64_t table_get;
    static uint64_t table_bloomfilter_miss;
    static uint64_t table_readfile_miss;
    static uint64_t table_cache_shoot;
    static uint64_t data_block_read;
    static uint64_t index_block_size;
    static uint64_t block_cache_read;
    static int open_num;
    static int get_flag;

    static int put_num;
    static int get_num;

    void getSnapShot(){
        readStaticDelta_.mem_get = ReadStatic::mem_get;
        for(int i =0;i<config::kNumLevels;i++)
           readStaticDelta_.level_get[i] = ReadStatic::level_get[i];

        readStaticDelta_.table_get = ReadStatic::table_get;
        readStaticDelta_.table_bloomfilter_miss = ReadStatic::table_bloomfilter_miss;
        readStaticDelta_.table_readfile_miss = ReadStatic::table_readfile_miss;
        readStaticDelta_.table_cache_shoot = ReadStatic::table_cache_shoot;
        readStaticDelta_.data_block_read = ReadStatic::data_block_read;
        readStaticDelta_.index_block_size = ReadStatic::index_block_size;
        readStaticDelta_.block_cache_read = ReadStatic::block_cache_read;
        readStaticDelta_.open_num = ReadStatic::open_num;
        readStaticDelta_.get_flag = ReadStatic::get_flag;

        readStaticDelta_.put_num = ReadStatic::put_num;
        readStaticDelta_.get_num = ReadStatic::get_num;

    }

    void getReadStaticDelta(){
        readStaticDelta_.mem_get = ReadStatic::mem_get - readStaticDelta_.mem_get;
        for(int i =0;i<config::kNumLevels;i++)
           readStaticDelta_.level_get[i] = ReadStatic::level_get[i] - readStaticDelta_.level_get[i];

        readStaticDelta_.table_get = ReadStatic::table_get - readStaticDelta_.table_get;
        readStaticDelta_.table_bloomfilter_miss =
                ReadStatic::table_bloomfilter_miss - readStaticDelta_.table_bloomfilter_miss;
        readStaticDelta_.table_readfile_miss = ReadStatic::table_readfile_miss - readStaticDelta_.table_readfile_miss;
        readStaticDelta_.table_cache_shoot = ReadStatic::table_cache_shoot - readStaticDelta_.table_cache_shoot;
        readStaticDelta_.data_block_read = ReadStatic::data_block_read - readStaticDelta_.data_block_read;
        readStaticDelta_.index_block_size = ReadStatic::index_block_size - readStaticDelta_.index_block_size;
        readStaticDelta_.block_cache_read = ReadStatic::block_cache_read - readStaticDelta_.block_cache_read;
        readStaticDelta_.open_num = ReadStatic::open_num - readStaticDelta_.open_num;
        readStaticDelta_.get_flag = ReadStatic::get_flag - readStaticDelta_.get_flag;

        readStaticDelta_.put_num = ReadStatic::put_num - readStaticDelta_.put_num;
        readStaticDelta_.get_num = ReadStatic::get_num - readStaticDelta_.get_num;

    }

    struct ReadStaticDelta readStaticDelta_;
};



// Value types encoded as the last component of internal keys.
// DO NOT CHANGE THESE ENUM VALUES: they are embedded in the on-disk
// data structures.
enum ValueType {
  kTypeDeletion = 0x0,
  kTypeValue = 0x1
};
// kValueTypeForSeek defines the ValueType that should be passed when
// constructing a ParsedInternalKey object for seeking to a particular
// sequence number (since we sort sequence numbers in decreasing order
// and the value type is embedded as the low 8 bits in the sequence
// number in internal keys, we need to use the highest-numbered
// ValueType, not the lowest).
static const ValueType kValueTypeForSeek = kTypeValue;

typedef uint64_t SequenceNumber;

// We leave eight bits empty at the bottom so a type and sequence#
// can be packed together into 64-bits.
static const SequenceNumber kMaxSequenceNumber =
    ((0x1ull << 56) - 1);

struct ParsedInternalKey {
  Slice user_key;
  SequenceNumber sequence;
  ValueType type;

  ParsedInternalKey() { }  // Intentionally left uninitialized (for speed)
  ParsedInternalKey(const Slice& u, const SequenceNumber& seq, ValueType t)
      : user_key(u), sequence(seq), type(t) { }
  std::string DebugString() const;
};

// Return the length of the encoding of "key".
inline size_t InternalKeyEncodingLength(const ParsedInternalKey& key) {
  return key.user_key.size() + 8;
}

// Append the serialization of "key" to *result.
extern void AppendInternalKey(std::string* result,
                              const ParsedInternalKey& key);

// Attempt to parse an internal key from "internal_key".  On success,
// stores the parsed data in "*result", and returns true.
//
// On error, returns false, leaves "*result" in an undefined state.
extern bool ParseInternalKey(const Slice& internal_key,
                             ParsedInternalKey* result);

// Returns the user key portion of an internal key.
inline Slice ExtractUserKey(const Slice& internal_key) {
  assert(internal_key.size() >= 8);
  return Slice(internal_key.data(), internal_key.size() - 8);
}

inline ValueType ExtractValueType(const Slice& internal_key) {
  assert(internal_key.size() >= 8);
  const size_t n = internal_key.size();
  uint64_t num = DecodeFixed64(internal_key.data() + n - 8);
  unsigned char c = num & 0xff;
  return static_cast<ValueType>(c);
}

// A comparator for internal keys that uses a specified comparator for
// the user key portion and breaks ties by decreasing sequence number.
class InternalKeyComparator : public Comparator {
 private:
  const Comparator* user_comparator_;
 public:
  explicit InternalKeyComparator(const Comparator* c) : user_comparator_(c) { }
  virtual const char* Name() const;
  virtual int Compare(const Slice& a, const Slice& b) const;
  virtual void FindShortestSeparator(
      std::string* start,
      const Slice& limit) const;
  virtual void FindShortSuccessor(std::string* key) const;

  const Comparator* user_comparator() const { return user_comparator_; }

  int Compare(const InternalKey& a, const InternalKey& b) const;
};

// Filter policy wrapper that converts from internal keys to user keys
class InternalFilterPolicy : public FilterPolicy {
 private:
  const FilterPolicy* const user_policy_;
 public:
  explicit InternalFilterPolicy(const FilterPolicy* p) : user_policy_(p) { }
  virtual const char* Name() const;
  virtual void CreateFilter(const Slice* keys, int n, std::string* dst) const;
  virtual bool KeyMayMatch(const Slice& key, const Slice& filter) const;
};

// Modules in this directory should keep internal keys wrapped inside
// the following class instead of plain strings so that we do not
// incorrectly use string comparisons instead of an InternalKeyComparator.
class InternalKey {
 private:
  std::string rep_;
 public:
  InternalKey() { }   // Leave rep_ as empty to indicate it is invalid
  InternalKey(const Slice& user_key, SequenceNumber s, ValueType t) {
    AppendInternalKey(&rep_, ParsedInternalKey(user_key, s, t));
  }

  void DecodeFrom(const Slice& s) { rep_.assign(s.data(), s.size()); }
  Slice Encode() const {
    assert(!rep_.empty());
    return rep_;
  }

  Slice user_key() const { return ExtractUserKey(rep_); }

  void SetFrom(const ParsedInternalKey& p) {
    rep_.clear();
    AppendInternalKey(&rep_, p);
  }

  void Clear() { rep_.clear(); }

  std::string DebugString() const;
  
  //whc add
  std::string Rep(){return rep_;}
};

inline int InternalKeyComparator::Compare(
    const InternalKey& a, const InternalKey& b) const {
  return Compare(a.Encode(), b.Encode());
}

inline bool ParseInternalKey(const Slice& internal_key,
                             ParsedInternalKey* result) {
  const size_t n = internal_key.size();
  if (n < 8) return false;
  uint64_t num = DecodeFixed64(internal_key.data() + n - 8);
  unsigned char c = num & 0xff;
  result->sequence = num >> 8;
  result->type = static_cast<ValueType>(c);
  result->user_key = Slice(internal_key.data(), n - 8);
  return (c <= static_cast<unsigned char>(kTypeValue));
}

// A helper class useful for DBImpl::Get()
class LookupKey {
 public:
  // Initialize *this for looking up user_key at a snapshot with
  // the specified sequence number.
  LookupKey(const Slice& user_key, SequenceNumber sequence);

  ~LookupKey();

  // Return a key suitable for lookup in a MemTable.
  Slice memtable_key() const { return Slice(start_, end_ - start_); }

  // Return an internal key (suitable for passing to an internal iterator)
  Slice internal_key() const { return Slice(kstart_, end_ - kstart_); }

  // Return the user key
  Slice user_key() const { return Slice(kstart_, end_ - kstart_ - 8); }

 private:
  // We construct a char array of the form:
  //    klength  varint32               <-- start_
  //    userkey  char[klength]          <-- kstart_
  //    tag      uint64
  //                                    <-- end_
  // The array is a suitable MemTable key.
  // The suffix starting with "userkey" can be used as an InternalKey.
  const char* start_;
  const char* kstart_;
  const char* end_;
  char space_[200];      // Avoid allocation for short keys

  // No copying allowed
  LookupKey(const LookupKey&);
  void operator=(const LookupKey&);
};

inline LookupKey::~LookupKey() {
  if (start_ != space_) delete[] start_;
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DBFORMAT_H_
