/*
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
*
*/

const { exec } = require("child_process");

exec("ps aux | grep kv_service | grep -v grep | head -n 1 | awk '{print $1}'", (err, stdout, stderr) => {
  if (err) {
    console.error(`Error finding kv_service process: ${stderr}`);
    return;
  }

  const pid = stdout.trim();

  if (pid) {
    console.log("PID of CPP Client-1", pid)
    const server_addr = process.env.PYROSCOPE_SERVER_ADDRESS  || "http://localhost:4040"
    const command = `pyroscope connect --server-address ${server_addr} --application-name cpp_client_1 --spy-name ebpfspy --pid ${pid}`;
    exec(command, (err, stdout, stderr) => {
      if (err) {
        console.error(`Error running pyroscope connect: ${stderr}`);
        return;
      }
      console.log(`Pyroscope connected successfully:\n${stdout}`);
    });
  } else {
    console.log("kv_service process not found.");
  }
});