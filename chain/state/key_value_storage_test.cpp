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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace resdb {
namespace {

TEST(KVServerExecutorTest, SetValue) {
  KeyValueStorage storage;

  EXPECT_EQ(storage.GetAllValues(), "[]");
  EXPECT_EQ(storage.SetValue("test_key", "test_value"), 0);
  EXPECT_EQ(storage.GetValue("test_key"), "test_value");

  // GetAllValues and GetRange may be out of order for in-memory, so we test up to
  // 1 key-value pair
  EXPECT_EQ(storage.GetAllValues(), "[test_value]");
  EXPECT_EQ(storage.GetRange("a", "z"), "[test_value]");
}

TEST(KVServerExecutorTest, GetValue) {
  KeyValueStorage storage;
  EXPECT_EQ(storage.GetValue("test_key"), "");
}

TEST(KVServerExecutorTest, GetEmptyValueWithVersion) {
  KeyValueStorage storage;
  EXPECT_EQ(storage.GetValueWithVersion("test_key", 0), 
    std::make_pair(std::string(""), 0));
}

TEST(KVServerExecutorTest, SetValueWithVersion) {
  KeyValueStorage storage;

  EXPECT_EQ(storage.SetValueWithVersion("test_key", "test_value", 1), -2);

  EXPECT_EQ(storage.SetValueWithVersion("test_key", "test_value", 0), 0);

  EXPECT_EQ(storage.GetValueWithVersion("test_key", 0), 
    std::make_pair(std::string("test_value"), 1));
  EXPECT_EQ(storage.GetValueWithVersion("test_key", 1), 
    std::make_pair(std::string("test_value"), 1));

  EXPECT_EQ(storage.SetValueWithVersion("test_key", "test_value_v2", 2), -2);
  EXPECT_EQ(storage.SetValueWithVersion("test_key", "test_value_v2", 1), 0);

  EXPECT_EQ(storage.GetValueWithVersion("test_key", 0), 
    std::make_pair(std::string("test_value_v2"), 2));

  EXPECT_EQ(storage.GetValueWithVersion("test_key", 1), 
    std::make_pair(std::string("test_value"), 1));

  EXPECT_EQ(storage.GetValueWithVersion("test_key", 2), 
    std::make_pair(std::string("test_value_v2"), 2));

  EXPECT_EQ(storage.GetValueWithVersion("test_key", 3), 
    std::make_pair(std::string("test_value_v2"), 2));
}

MATCHER_P(EqualsValueList, expect_list, "") {
  sort(expect_list.begin(), expect_list.end());
  return expect_list == arg;
  /*
  if(expect_list.size() != arg.size()){
    return false;
  }
  */
  //return ::google::protobuf::util::MessageDifferencer::Equals(arg, replica);
}



TEST(KVServerExecutorTest, GetAllValueWithVersion) {
  KeyValueStorage storage;

  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("test_key", std::make_pair("test_value",1))
    };

    EXPECT_EQ(storage.SetValueWithVersion("test_key", "test_value", 0), 0);
    EXPECT_EQ(storage.GetAllValuesWithVersion(), expected_list);
  }

  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("test_key", std::make_pair("test_value_v2",2))
    };
    EXPECT_EQ(storage.SetValueWithVersion("test_key", "test_value_v2", 1), 0);
    EXPECT_EQ(storage.GetAllValuesWithVersion(), expected_list);
  }

  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("test_key_v1", std::make_pair("test_value1",1)),
      std::make_pair("test_key", std::make_pair("test_value_v2",2))
    };
    EXPECT_EQ(storage.SetValueWithVersion("test_key_v1", "test_value1", 0), 0);
    EXPECT_EQ(storage.GetAllValuesWithVersion(), expected_list);
  }
}

TEST(KVServerExecutorTest, GetRangeWithVersion) {
  KeyValueStorage storage;

  EXPECT_EQ(storage.SetValueWithVersion("1", "value1", 0), 0);
  EXPECT_EQ(storage.SetValueWithVersion("2", "value2", 0), 0);
  EXPECT_EQ(storage.SetValueWithVersion("3", "value3", 0), 0);
  EXPECT_EQ(storage.SetValueWithVersion("4", "value4", 0), 0);

  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("1", std::make_pair("value1",1)),
      std::make_pair("2", std::make_pair("value2",1)),
      std::make_pair("3", std::make_pair("value3",1)),
      std::make_pair("4", std::make_pair("value4",1))
    };
    EXPECT_EQ(storage.GetRangeWithVersion("1", "4"), expected_list);
  }

  EXPECT_EQ(storage.SetValueWithVersion("3", "value3_1", 1), 0);
  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("1", std::make_pair("value1",1)),
      std::make_pair("2", std::make_pair("value2",1)),
      std::make_pair("3", std::make_pair("value3_1",2)),
      std::make_pair("4", std::make_pair("value4",1))
    };
    EXPECT_EQ(storage.GetRangeWithVersion("1", "4"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("1", std::make_pair("value1",1)),
      std::make_pair("2", std::make_pair("value2",1)),
      std::make_pair("3", std::make_pair("value3_1",2)),
    };
    EXPECT_EQ(storage.GetRangeWithVersion("1", "3"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("2", std::make_pair("value2",1)),
      std::make_pair("3", std::make_pair("value3_1",2)),
      std::make_pair("4", std::make_pair("value4",1))
    };
    EXPECT_EQ(storage.GetRangeWithVersion("2", "4"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("1", std::make_pair("value1",1)),
      std::make_pair("2", std::make_pair("value2",1)),
      std::make_pair("3", std::make_pair("value3_1",2)),
      std::make_pair("4", std::make_pair("value4",1))
    };
    EXPECT_EQ(storage.GetRangeWithVersion("0", "5"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list {
      std::make_pair("2", std::make_pair("value2",1)),
      std::make_pair("3", std::make_pair("value3_1",2)),
    };
    EXPECT_EQ(storage.GetRangeWithVersion("2", "3"), expected_list);
  }
}

TEST(KVServerExecutorTest, GetHistory) {
  KeyValueStorage storage;

  {
  std::vector<std::pair<std::string, int> > expected_list {
    };
    EXPECT_EQ(storage.GetHistory("1", 1,5), expected_list);
  }
  {
    std::vector<std::pair<std::string, int> > expected_list {
      std::make_pair("value1",1),
      std::make_pair("value2",2),
      std::make_pair("value3",3)
    };

    EXPECT_EQ(storage.SetValueWithVersion("1", "value1", 0), 0);
    EXPECT_EQ(storage.SetValueWithVersion("1", "value2", 1), 0);
    EXPECT_EQ(storage.SetValueWithVersion("1", "value3", 2), 0);

    EXPECT_EQ(storage.GetHistory("1", 1,5), expected_list);
  }

  {
    std::vector<std::pair<std::string, int> > expected_list {
      std::make_pair("value1",1),
      std::make_pair("value2",2),
      std::make_pair("value3",3),
      std::make_pair("value4",4),
      std::make_pair("value5",5)
    };

    EXPECT_EQ(storage.SetValueWithVersion("1", "value4", 3), 0);
    EXPECT_EQ(storage.SetValueWithVersion("1", "value5", 4), 0);
    EXPECT_EQ(storage.SetValueWithVersion("1", "value6", 5), 0);
    EXPECT_EQ(storage.SetValueWithVersion("1", "value7", 6), 0);

    EXPECT_EQ(storage.GetHistory("1", 1,5), expected_list);
  }
}





}  // namespace

}  // namespace resdb
