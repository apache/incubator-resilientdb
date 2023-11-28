/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "chain/state/key_value_storage.h"

#include <glog/logging.h>

namespace resdb {

KeyValueStorage::KeyValueStorage() {}

int KeyValueStorage::SetValue(const std::string& key, const std::string& value) {
    if(kv_map_with_v_[key].size()){
      LOG(ERROR)<<"invalid zero version. key:"<<key<<" version has been set.";
      return -2;
    }
    kv_map_[key]=value;
    return 0;
}

std::string KeyValueStorage::GetAllValues(void) {
  std::string values = "[";
  bool first_iteration = true;
  for (auto kv : kv_map_) {
    if (!first_iteration) values.append(",");
    first_iteration = false;
    values.append(kv.second);
  }
  values.append("]");
  return values;
}

std::string KeyValueStorage::GetRange(const std::string& min_key,
                                 const std::string& max_key) {
  std::string values = "[";
  bool first_iteration = true;
  for (auto kv : kv_map_) {
    if (kv.first >= min_key && kv.first <= max_key) {
      if (!first_iteration) values.append(",");
      first_iteration = false;
      values.append(kv.second);
    }
  }
  values.append("]");
  return values;
}

std::string KeyValueStorage::GetValue(const std::string& key) {
  auto search = kv_map_.find(key);
  if (search != kv_map_.end())
    return search->second;
  else {
    return "";
  }
}


int KeyValueStorage::SetValueWithVersion(
    const std::string& key, const std::string& value, int version) {
  auto it = kv_map_with_v_.find(key);
  if((it == kv_map_with_v_.end() && version != 0) 
      || (it != kv_map_with_v_.end() && it->second.back().second != version)){
    LOG(ERROR)<<" value version not match. key:"<<key<<
      " db version:"<<(it == kv_map_with_v_.end()?0:it->second.back().second) <<
      " user version:"<<version;
    return -2;
  }
  kv_map_with_v_[key].push_back(std::make_pair(value,version+1));
  return 0;
}


std::pair<std::string,int> KeyValueStorage::GetValueWithVersion(const std::string& key, int version) {
  auto search_it = kv_map_with_v_.find(key);
  if (search_it != kv_map_with_v_.end() && search_it->second.size()){
    auto it = search_it->second.end();
    do{
      --it;
      if( it->second == version ){
        return *it;
      }
      if(it->second < version) {
        break;
      }
    }while(it!=search_it->second.begin());
    it = --search_it->second.end();
    LOG(ERROR)<<" key:"<<key<<" no version:"<<version<<" return max:"<<it->second;
    return *it;
  }
  return std::make_pair("",0);
}

std::map<std::string, std::pair<std::string, int>>
    KeyValueStorage::GetAllValuesWithVersion() {
  std::map<std::string,std::pair<std::string,int>> resp;

  for(const auto& it : kv_map_with_v_){
    resp.insert(std::make_pair(it.first, it.second.back()));
  }
  return resp;
}

std::map<std::string,std::pair<std::string,int>>
  KeyValueStorage::GetRangeWithVersion(
    const std::string& min_key, const std::string& max_key) {

  std::map<std::string, std::pair<std::string,int>> resp;
  for(const auto& it : kv_map_with_v_){
    if (it.first >= min_key && it.first <= max_key){
      resp.insert(std::make_pair(it.first, it.second.back()));
    }
  }
  return resp;
}

std::vector<std::pair<std::string,int>> KeyValueStorage::GetHistory(
    const std::string& key, const int& min_version, const int& max_version) {

  std::vector<std::pair<std::string,int>> resp;
  auto search_it = kv_map_with_v_.find(key);
  if(search_it == kv_map_with_v_.end()){
    return resp;
  }

  for(const auto& it : search_it->second) {
    if(it.second >= min_version && it.second <= max_version){
      resp.push_back(it);
    }
  }
  return resp;


}

}  // namespace resdb
