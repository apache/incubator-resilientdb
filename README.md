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
--> 

Forked from [this repository](https://github.com/apache/incubator-resilientdb), for more complex setup instructions, please head there.

# (In Progress) How to Setup The Repository
Hey all, Steven here, this is the quickstart guide to getting the Indexer project up and running.

1. Clone it to your local device with `git clone https://github.com/apache/incubator-resilientdb.git`

2. (Windows only) ResilientDB uses bash shell commands (.sh extension), which windows doesn't support natively (pretty sure mac _DOES_). Fortunately, Windows 11 and most versions of Windows 10 have an easy to use subsystem for Linux, WSL. Link on how to setup [here](https://learn.microsoft.com/en-us/windows/wsl/install).
After installing WSL, you can open a bash terminal by running the program `Ubuntu`. This will open from the profile of your newly created User for WSL, but you can still access to your Windows files in windows via `cd ~/../../mnt`, which should navigate you to the location of your C/D drive.

3. Navigate to the project folder and run `sudo sh INSTALL.sh`

### Windows - Linux Mismatch

If you ran the command above on Windows/Ubuntu, and you recieved at least one error message, you're cooked. I've spent the last few hours trying to debug and figure out what happened, and I THINK this is because of a mismatch between the way Windows and Linux/Mac handle newlines vs carraige returns. As I understand it, `INSTALL.sh` effectively adds an unncessary carraige return character (\r) to the end of every line, which messes with file names.

#### The Main Fix
To clean this up, we need to make the install file Bash friendly again. To do this, I ran both `sed -i 's/\r//g' INSTALL.sh` (a simple character replacement program) and `dos2unix your_script.sh` (an advanced program built for this exact purpose, you may have to install it with apt). Your miles may vary, and likely this will work with only one of the two commands.

After this, try to run `sudo sh INSTALL.sh` again and see if you get any of the following errors:

#### Missing Bazel Version
This looks something like `(specified in /mnt/c/Users/username/Desktop/indexers-ECS265-Fall2025/.bazelversion), but it wasn't found in /usr/bin.`

This should go away if you delete this .bazelversion in your `indexers-ECS265-Fall2025` folder

#### Invalid Filename Extension
This looks something like `Ignoring file 'bazel.list ' in directory '/etc/apt/sources.list.d/' as it has an invalid filename extension`

I believe this happens becuase bazel (the build tool used by ResDB) tries to save it's version into the file `bazel.list\r`, accidentally adding a cairrage return to the end.

To fix this, go to the repository it listed `cd /etc/apt/sources.list.d`, and look at the files with `ls`. You should see two: `bazel.list` and another weirdly formatted one (it came out as `bazel.list^M` for me). Remove them both using `sudo rm`.

#### Using the right version of gcc and g++
If the install script runs, but the start_kv service tool starts to display a ton of errors with what looks like c++ code, you most likely need to change the version of c++ that bazel is using.

**Basic Fix**
Yoshiki found that gcc and g++ 12 work best with Resilient DB. If you're getting these errors, run the following commands:

```
sudo apt install gcc-12 g++-12
export CC=/usr/bin/gcc-12
export CXX=/usr/bin/g++-12
bazel clean
```

re-run the INSTALL script, then try to run the start_kv service

**Advanced Fix**
This alone didn't work for me, and I needed to tell my whole Ubuntu distribution to use gcc-12 and g++-12 by default.

Run these commands to let your device know that version 12 is a usable version of gcc. Note that you need to run `sudo apt install gcc-12 g++-12` to get them on your device first.

`sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 90`
`sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 90`

Note that the last two numbers (90) are an arbitrary priority value. You can set them to whatever you want.

Now run both commands:

`sudo update-alternatives --config gcc`
`sudo update-alternatives --config g++`

Both should bring up an list of allowed versions of gcc/g++, with a pointer pointing to your Ubuntu's default version of each. For both, select the number associated with version 12.

Verify that you're now using version 12 for both:

`gcc --version`
`g++ --version`

Re-running the INSTALL and kv_start scripts should work now.

This took me a couple tries to get right, and mistakes with `update-alternatives` were tough to recover from. Uninstalling WSL/Ubuntu then reinstalling it fresh always gets a fresh version of gcc / g++ that works again. Note that this will remove everything in your _Ubuntu_ distro (not everything on your computer)