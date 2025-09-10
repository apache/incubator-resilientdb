/*

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

#include <gtest/gtest.h>

#include "common/test/test.pb.h"
#include "common/test/test_macros.h"

namespace resdb {
namespace testing {
namespace {

using ::resdb::testing::EqualsProto;

TEST(SignatureVerifyTest, ParseFromJson) {
  resdb::test::JsonMessage json_msg;

  json_msg.set_id(1);
  json_msg.set_data("data");
  json_msg.mutable_sub_msg()->set_id(2);
  json_msg.mutable_sub_msg()->set_data("sub_data");

  std::string json =
      "{ \
			    \"id\": 1, \
			    \"data\": \"data\", \
			    \"sub_msg\":{ \
				\"id\": 2, \
			        \"data\":\"sub_data\" \
			    } \
	}";

  EXPECT_THAT(ParseFromText<resdb::test::JsonMessage>(json),
              EqualsProto(json_msg));
}

}  // namespace
}  // namespace testing
}  // namespace resdb
