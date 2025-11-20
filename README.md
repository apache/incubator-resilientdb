<!--
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing,
 software distributed under the License is distributed on an
 "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND, either express or implied.  See the License for the
 specific language governing permissions and limitations
 under the License.

 - - - - -

 This specific repository is forked from the development build
 of ResilientDB for a Fall 2025 UC Davis graduate class project.

 Participants: Steven Shoemaker, Tiching Kao, Regan Yang,
 Yoshiki Yamaguchi, Ritesh Patro
--> 

# Table of Contents
1. [Running the Indexing Project](#Running-the-Indexing-Project)
2. [ResilientDB Installation](#ResilientDB-Installation)
3. [ResilientDB Installation Bugs](#ResilientDB-Installation-Bugs)
4. [How to Run ResDB-ORM](#How-to-Run-ResDB-ORM)

## Running the Indexing Project
TODO

\

## ResilientDB Installation
Forked from [this repository](https://github.com/apache/incubator-resilientdb), for more complex setup instructions, please head there.

Hey all, Steven here, this is the quickstart guide to getting ResDB up and running. If you've already setup and installed your repo, start from step 5.

1. Clone this repo to your local device with `git clone https://github.com/apache/incubator-resilientdb.git`

2. (Windows only) ResilientDB uses bash shell commands (.sh extension), which windows doesn't support natively. Fortunately, Windows 11 and most versions of Windows 10 have an easy to use subsystem for Linux, WSL. Link on how to setup [here](https://learn.microsoft.com/en-us/windows/wsl/install).
After installing WSL, you can open a bash terminal by running the program `Ubuntu`. This will open from the profile of your newly created User for WSL, but you can still access to your Windows files in windows via `cd ~/../../mnt`, which should navigate you to the location of your C/D drive.

3. (Windows only?) There's a mismatch between the way Windows and Linux ends lines in files, in short, on Windows machines the shell scripts will all have an unnecessary `\r` (carriage return) character at the end of all shell files. This _will_ cause problems with execution of these files. Use the sed command (at the top-level of the cloned repo) to remove the extraneous characters:

```
sudo sed -i 's/\r//g' INSTALL.sh
sudo sed -i 's/\r//g' ./service/tools/kv/server_tools/start_kv_service.sh
```

4. Navigate to the project folder and run `sudo sh INSTALL.sh`

5. To start the k/v store, run `./service/tools/kv/server_tools/start_kv_service.sh`

6. To start tools for the k/v store, run `bazel build service/tools/kv/api_tools/kv_service_tools`

If you're starting from step 1, you'll more likely than not run into bugs. Here are a list of ones we've come across and their fixes:

\

## ResilientDB Installation Bugs
### Carriage returns & running shell files on Windows
For Windows (and mac?) users, we need to make bash files friendly for your OS. To do this, we can just run a simple character replacement program on any shell files, `sed -i 's/\r//g' YOUR_SHELL_SCRIPT.sh`. We talk about doing this for INSTALL.sh and start_kv_service.sh in the Installation guide, but it will need to be done for any shell file you want to run. For issues with sed, instead run and `dos2unix YOUR_SHELL_SCRIPT.sh`

\

### Socket Closed
We found that this is likely an issue of WSL not being allocated enough resources.

(Windows Only) Shut off your WSL (`wsl --shutdown`). Navigate to %UserProfile%/.wslconfig, and replace the text in that file with the following:

```
[wsl2]
memory=6GB
processors=4

```

(or as close as you can get, in accordance with your device's capabilities)

\

### Missing Bazel Version
This looks something like `(specified in /mnt/c/Users/username/Desktop/indexers-ECS265-Fall2025/.bazelversion), but it wasn't found in /usr/bin.`

This goes away if you delete the .bazelversion in your `indexers-ECS265-Fall2025` folder. The file should no longer be there.

This project strictly requires **Bazel 6.0.0**. If you have accidentally upgraded to a newer version (e.g., via `apt upgrade`) and need to revert it manually, follow these steps:

1.  **Remove the current Bazel version**
    ```bash
    sudo apt-get remove bazel
    # If previously installed manually, remove the binary directly:
    # sudo rm /usr/local/bin/bazel
    ```

2.  **Download Bazel 6.0.0**
    ```bash
    wget https://github.com/bazelbuild/bazel/releases/download/6.0.0/bazel-6.0.0-linux-x86_64 -O bazel
    ```

3.  **Install to system path**
    ```bash
    chmod +x bazel
    sudo mv bazel /usr/local/bin/bazel
    ```

4.  **Refresh shell cache and verify**
    It is important to clear the shell's location cache so it finds the new binary.
    ```bash
    hash -r
    bazel --version
    # Output should be: bazel 6.0.0
    ```
\

### Invalid Filename Extension
This looks something like `Ignoring file 'bazel.list ' in directory '/etc/apt/sources.list.d/' as it has an invalid filename extension`

This happens becuase bazel (the build tool used by ResDB) tries to save it's version into the file `bazel.list\r`, accidentally adding a cairrage return to the end. This is a common problem when running INSTALL.sh without deleting the cairrage returns with sed.

To fix this, go to the repository it listed `cd /etc/apt/sources.list.d`, and look at the files with `ls`. You should see two: `bazel.list` and another weirdly formatted one (it came out as `bazel.list^M` for me). Remove them both using `sudo rm`. You'll need to run INSTALL.sh again after this step.

\

### Problems with /root/.cache/bazel
Sometimes when running `start_kv_service.sh`, you may face this problem. It encompasses any errors with /.cache/bazel. We _think_ it's a side-effect of trying to run the script with the wrong version of gcc/g++, causing bazel to cache incorrect information, and use that information even when later operating with gcc/g++ versions.

First, from the top-level of your repo, run `bazel clean`. Then, remove the entire bazel folder from the cache, `sudo rm -r /root/.cache/bazel` (in some cases, this may be in your home/user directory, run the command in accordance with wherever your error is reporting from). Please make sure your gcc/g++ distribution is the correct version after this (the step below this one).

Typically, the /.cache/ is safe to delete from, but we don't understand it's function very well. If you have concerns, look into the purpose of the cache on your own time and determine your willingness to perform this step.

\

### Using the right version of gcc and g++
If the install script runs, but the start_kv service tool starts to display a ton of errors with what looks like c++ code, you most likely need to change the version of gcc/g++ that bazel is using.

**Basic Fix**\
Yoshiki found that gcc and g++ 12 work best with Resilient DB. If you're getting these errors, run the following commands:

```
sudo apt install gcc-12 g++-12
export CC=/usr/bin/gcc-12
export CXX=/usr/bin/g++-12
bazel clean
```

re-run the INSTALL script, then try to run the start_kv service

**Advanced Fix**\
This alone didn't work for me, and I needed to tell my whole Ubuntu distribution to use gcc-12 and g++-12 by default.

Run these commands to let your device know that version 12 is a usable version of gcc. Note that you need to run `sudo apt install gcc-12 g++-12` to get them on your device first.

```
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 90
```

Note that the last two numbers (90) are an arbitrary priority value. You can set them to whatever you want.

Now run both commands:

```
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
```

Both should bring up an list of allowed versions of gcc/g++, with a pointer pointing to your Ubuntu's default version of each. For both, select the number associated with version 12.

Verify that you're now using version 12 for both:

```
gcc --version
g++ --version
```

Re-running the INSTALL and kv_start scripts should work now.

This took me a couple tries to get right, and mistakes with `update-alternatives` were tough to recover from. Uninstalling WSL/Ubuntu then reinstalling it fresh always gets a fresh version of gcc / g++ that works again. Note that this will remove everything in your _Ubuntu_ distro (not everything on your computer)

## How to Run ResDB-ORM

To run ResDB-ORM, you must first start the backend services (**KV Service** and **GraphQL Server**) and then connect to them using **ResDB-ORM**.

### Step 1: Start the KV Service
Run the following script in your ResilientDB directory:
```bash
./service/tools/kv/server_tools/start_kv_service.sh
```

### Step 2: Start the GraphQL Server
Open a new terminal tab, then setup and start the GraphQL server:
(1) Clone the repository and navigate into it:
```bash
git clone https://github.com/apache/incubator-resilientdb-graphql.git
cd incubator-resilientdb-graphql
```
(2) Create and activate a virtual environment:
```bash
python3.10 -m venv venv
```
(3) Build and run the service:
```bash
bazel build service/http_server:crow_service_main
bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config
```
***Important:*** Check the first line of the startup log and copy the displayed URL (e.g., ```http://0.0.0.0:18000```). You will need this for the configuration step.

### Step 3: Clone ResDB-ORM repository and install dependencies:
Open another new terminal tab to set up the ORM and verify the operation.
```bash
git clone https://github.com/apache/incubator-resilientdb-ResDB-ORM.git
cd ResDB-ORM

python3.10 -m venv venv
source venv/bin/activate

pip install -r requirements.txt
pip install resdb-orm
```

### Step 4: Open ```config.yaml``` and update the db_root_url with the GraphQL Server URL you copied in Step 2.
```yaml
database:
  db_root_url: <CROW_ENDPOINT>  
```

### Step 5: Run the test script to ensure everything is working correctly:
```bash
python tests/test.py
```
