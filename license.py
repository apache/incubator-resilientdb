import sys
import os


current_copyright = """/*
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
 */"""

new_copyright = """/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
* 
*   http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.    
*/
"""

cp = current_copyright.split('\n')
ncp = new_copyright.split('\n')

def replace_license(file_name):

  lines = []
  newlines = []
  with open(file_name) as f:
    lines = f.readlines()
  
  def replace():
    nonlocal newlines
    for i in range(len(lines)):
      for j in range(i,len(lines)):
        idx = 0
        match = True 
        for k in range(i,j+1):
          if cp[idx].strip() == lines[k].strip():
            idx = idx + 1
            continue
          match = False
        if match and idx == len(cp):
          newlines = lines[0:i] + [new_copyright] + lines[j+1:]
          return

  replace()
  if len(newlines) > 0:
    with open(file_name,"w") as f:
      print("write new line:",file_name)
      f.writelines(newlines)

def replace_file(file_name):
  #print("replca file:",file_name)
  subfix = os.path.splitext(file_name)[-1];
  if(subfix == ".cert" or subfix == ".pub" or subfix == ".pri"):
    return
  has_copyright = False
  with open(file_name) as f:
    for line in f.readlines():
      if line.find("Copyright") >= 0:
        has_copyright = True
  
  if has_copyright:
    #print("need:",file_name)
    replace_license(file_name)

def scan_dir(dir_name):
  for name in os.listdir(dir_name):
    full_name = os.path.join(dir_name, name)
    if os.path.isfile(full_name):
      replace_file(full_name)
    else:
      scan_dir(full_name)

if __name__ == '__main__':
  dir_name = sys.argv[1]
  scan_dir(dir_name)
