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

Gem::Specification.new do |spec|
  spec.name          = 'res_cli'
  spec.version       = '0.1.0'
  spec.authors       = ['gopuman']
  spec.email         = ['gopalnambiar2@gmail.com']
  spec.summary       = 'CLI to manage ResilientDB and Python SDK instances'
  spec.description   = 'A command-line interface (CLI) to manage ResilientDB and Python SDK instances.'    
  spec.homepage      = 'https://github.com/ResilientApp/ResCLI'
  spec.license       = 'APSL-2.0'
  spec.executables   = ['res_cli']
  
  spec.files         = Dir['lib/**/*', 'config.ini']
  spec.add_runtime_dependency 'open3'
  spec.add_runtime_dependency 'optparse'
  spec.add_runtime_dependency 'inifile'
  spec.require_paths = ['lib']
end