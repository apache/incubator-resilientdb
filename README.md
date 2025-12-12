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
1. [First-Time Installation](#First-Time-Installation)
2. [Using the Project](#Using-the-Project)
3. [Stress testing the Project](#Stress-testing-the-Project)
4. [(Appendix) Common Installation bugs](#(Appendix)-Common-Installation-Bugs)

## First-Time Installation
Forked from [this repository](https://github.com/apache/incubator-resilientdb), for more complex setup instructions, please head there.

Hey all, Steven here. This is the quickstart guide to getting this project up and running. Vector Indexing itself only requires one line of setup (`pip install hnswlib sentence-transformers numpy`, from your venv or with python), but it is built on top of ResDB tooling that _does_ require setup (kv_store, graphQL server, and graphQL itself). This guide will walk you through setting those elements up for the first time.

1. Clone this repo to your local device with `git clone https://github.com/apache/incubator-resilientdb.git`

2. (Windows only) ResilientDB uses bash shell commands (.sh extension), which windows doesn't support natively. Fortunately, Windows 11 and most versions of Windows 10 have an easy to use subsystem for Linux, WSL. Link on how to setup [here](https://learn.microsoft.com/en-us/windows/wsl/install).
After installing WSL, you can open a bash terminal by running the program `Ubuntu`. This will open from the profile of your newly created User for WSL, but you can still access to your Windows files in windows via `cd ~/../../mnt`, which should navigate you to the location of your C/D drive.

3. (Windows/WSL only) There's a mismatch between the way Windows and Linux ends lines in files, in short, on Windows machines the shell scripts will all have an unnecessary `\r` (carriage return) character at the end of all shell files. This _will_ cause problems with execution of these files. Use the sed command (at the top-level of the cloned repo) to remove the extraneous characters of the install file:

```bash
sudo sed -i 's/\r//g' INSTALL.sh
```

Unfortunately, this is a problem with every shell file in the repository. Instead of just running the above command, we reccomend running it in the top-level directory and having the change propagate to all shell files: 

```bash
find . -type f -name '*sh' -exec sed -i 's/\r//g' {} \;
```

4. Navigate to the project folder and run the install script
```bash
sudo sh INSTALL.sh
```

5. The first component the indexing project is built upon is the kv_store. It is reccomended that you do this step in a seperate command line, as it can take control of the command line while running. From the top level of your project directory, run:

```bash
./service/tools/kv/server_tools/start_kv_service.sh
```

Reciving one or more `nohup: redirecting stderr to stdout` messages indicates that the service is running.

6. The second component the indexing project is built on is the graphql server. It is reccomended that you do this step in a seperate command line, as it can take control of the command line while running. Navigate to `ecosystem/graphql`, and run the following commands:

```bash
# First-time installation only
bazel build service/http_server:
# Start the server
bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config
```

The first command may take some time to run. Reciving one or more `[INFO    ]` messages indicates that the service is running.

7. (optional) The third component that the indexing project is built on is the graphql tool itself. This server runs on python. It is a requirement of this project that python3.10 is used (see [the appendix](#(Appendix)-Common-Installation-Bugs) for help using different python versions). While you can run this from your device's global python distribution, it is reccomended that you use a venv, as following:

```bash
python3.10 -m venv venv
source venv/bin/activate
```

to leave the virtual environment, just run `deactivate`

8. We need to run installation for grapql tooling. First navigate to `ecosystem/graphql`. Then, from your python venv or global python distribution, run:

```bash
pip install -r requirements.txt
pip install hnswlib sentence-transformers numpy
```

Note that these commands can take a very long time to run (10+ minutes)

Next, we need to start graphql. It is reccomended that you do this step in a seperate command line, as it can take control of the command line while running. While still in the `ecosystem/graphql`, and run the following command:

```bash
python app.py
```

## Using the Project
### Starting Back Up Again
Every time you would like to use the project again, you need to again run the kv_store, graphql server and graphql itself:

```bash
# From the top-level directory
./service/tools/kv/server_tools/start_kv_service.sh
# From ecosystem/graphql
bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config
# From ecosystem/graphql, AND using your python virtual environment (if applicable)
python app.py
```

Note that running each of these will most likely take away control of the terminal.

### Commands
All of the functionality of this program is accessible through a CLI tool located in `ecosystem/sdk/vector-indexing/kv_vector.py`

#### Adding a Value
From `ecosystem/sdk/vector-indexing`, run the command

```bash
python kv_vector.py --add <YOUR_STRING>
```

This will save both the value, as well an embedding representing the value, to ResDB.

- `YOUR_STRING` will be saved as a string, regardless of the format it is sent as. Duplicate values cannot be saved.

#### Deleting a Value
From `ecosystem/sdk/vector-indexing`, run the command

```bash
python kv_vector.py --delete <YOUR_STRING>
```

This remove both the value, as well the embedding representing the value, from ResDB. If `YOUR_STRING` has not been saved through our tooling, nothing will happen.

#### Searching Across Embeddings
This is the main functionality of our project - the ability to search for the most similar values based on their embeddings. To get the k-closest values to a queried string, run:

```bash
python kv_vector.py --get <YOUR_STRING> --k_matches <YOUR_INTEGER>
```

This will return the `YOUR_INTEGER` most similar currently stored values to `YOUR_STRING`, as long as their similarity score.

- If `--k_matches` is omitted, a value of k=1 will be used. If a non-integer value is used for this input, the program will terminate.

There is also a command to see all values saved to ResDB that this tool has generated an embedding for:

```bash
python kv_vector.py --getAll
```

#### Other
To get a brief recap of all of this functionality, you can run:

```bash
python kv_vector.py --help
```

## Stress Testing the Project

We tested for the storage limit of big values. In this configuration: 
1. 8GB RAM Shell
2. Standard 5 replica config from `./service/tools/kv/server_tools/start_kv_service.sh`

The results was that around 150-200mb values will cause the KV store to have long delays on operations. You can read more in `hnsw-test/index_test/README.md` along with the testing kit. 

## (Appendix) Common Installation Bugs

### Using Python3.10
The project will not be able to install the correct dependencies for graphql if a version aside from python3.10 is used. Specifically, python3.10 needs to create the virtual environment, or be the globally install version of python if all commands are run outside of a venv.

There are several ways of doing this, but we reccomend using deadsnakes

```bash
sudo apt install software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt install python3.10 python3.10-dev python3.10-venv
```

This will create a command in your terminal, `python3.10`, which can be used to create the venv.

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
