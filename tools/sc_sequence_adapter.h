// Optional donor sequence-format adapter; authored by djultra64.
// Preserve original63 context layout and opcode execution. Importer owns range lifetimes.
#pragma once
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <vector>
namespace clay_sc {
class SequenceFormats {
 struct Range { uint32_t begin,end; };
 std::vector<Range> ranges_;
 mutable std::mutex mutex_;
public:
 void register_definition(unsigned id,uint32_t begin,uint32_t decoded_size) {
  if((id!=6&&id!=11&&id!=12&&id!=16)||begin<0x80000000u||decoded_size==0||
     uint64_t(begin)+decoded_size>0x80800000ull)
   throw std::invalid_argument("Invalid optional Sculptor's Cut character definition");
  std::lock_guard lock(mutex_);
  for(auto&r:ranges_)if(r.begin==begin){r.end=begin+decoded_size;return;}
  ranges_.push_back({begin,begin+decoded_size});
 }
 void unregister_definition(uint32_t begin) {
  std::lock_guard lock(mutex_);
  for(auto it=ranges_.begin();it!=ranges_.end();++it)if(it->begin==begin){ranges_.erase(it);return;}
 }
 bool registered_definition(uint32_t begin) const {
  std::lock_guard lock(mutex_);
  for(const auto& r:ranges_)if(r.begin==begin)return true;
  return false;
 }
 int header_bytes(uint32_t sequence) const {
  std::lock_guard lock(mutex_);
  for(const auto&r:ranges_)if(sequence>=r.begin&&uint64_t(sequence)+16<=r.end)return 16;
  return 14;
 }
};
}
