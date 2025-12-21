# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import requests
from resdb_orm.orm import ResDBORM

db = ResDBORM()

# Create records
data = {"name": "abc", "age": 123}
create_response = db.create(data)
print("Create Response:", create_response)

# Retrieve records
read_response = db.read(create_response)
print("Read Response:", read_response)
print("Data:", read_response["data"])

# Update records
update_response = db.update(create_response, {"name": "def", "age": 456})
print("Update Response:", update_response)
read_response = db.read(create_response)
print("Read Response:", read_response)
print("New Data:", read_response["data"])

# Delete records
delete_response = db.delete(create_response)
print("Delete Response:", delete_response)
read_response = db.read(create_response)

