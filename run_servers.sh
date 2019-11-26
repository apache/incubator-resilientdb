#------------------------------------------------------------------------
# run_servers.sh
# 
# Properly exits existing/running ExpoDB server instances, boots up fresh
# docker instances, and automatically runs the code on each server
#
# Requirements: Place/run in ExpoDB-BC folder, along with Rohan's
#               createIpConfig.sh
#
# Options: -start      - Boots up containers and runs ExpoDB on servers
#          -rerun      - Begins a new run without rebooting everything
#          -sleep      - Exits processes but maintains containers
#          -stop       - Exits processes and shuts down containers
#
# Extra:   -c          - Use to recompile rundb and runcl
#
# Date:   11/9/2019
# Author: Julian T. Angeles
#------------------------------------------------------------------------

#! /bin/bash

# Set initial options
unset GREP_OPTIONS
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Initialize variable defaults
start=false
rerun=false
sleep=false
stop=false

make=false

replicas=4
clients=1

dependency="scripts/docker-ifconfig.sh"

os="${OSTYPE}"

# Check for proper usage and set option if correct
if [ "${1}" = "-start" ] ; then
	start=true
elif [ "${1}" = "-rerun" ] ; then
	rerun=true
elif [ "${1}" = "-sleep" ] ; then
	sleep=true
elif [ "${1}" = "-stop" ] ; then
	stop=true
else
        echo "Incorrect usage. Use by: ./run_servers.sh [option] [-c]"
	echo "Options (-start, -rerun, -sleep, -stop)"
	echo "         -c     Use to recompile executables"
        echo "Exiting..."

	exit 1
fi

if [ "${2}" = "-c" ] ; then
	make=true
fi

# Check if in the correct directory
if [[ ! $PWD =~ (resilientdb)$ ]] ; then
	echo "In the wrong directory. Run in ExpoDB-BC. Exiting..."

	exit 1
fi

# Check that createIpConfig.sh exists
if [[ ! -f $dependency ]] ; then
	echo "Missing $dependency in directory. Exiting..."

	exit 1
fi

# Check if docker command exists
if ! hash docker 2> /dev/null; then
  echo "Docker command not found. Please install Docker"
  echo "Exiting..."

  exit 1
fi

# Check for existing/running processes (start,rerun,sleep,stop)
pids=$(ps -ef | grep -E "./run(db|cl) -nid[0-9]+" | sort -r | awk '{print $2}')
if [[ ! -z "${pids}" ]] ; then     # Exit processes 

	echo "Closing existing docker processes..."

	# Exit running programs on servers
	for i in $(seq 1 $replicas); do
                docker exec s${i} pkill rundb	
	done

	for i in $(seq 1 $clients); do
                docker exec c${i} pkill runcl
	done

	#echo "${pids[@]}"

	# Exit all child processes
        if [[ ${os} == "linux-gnu" ]]; then
	for parent in ${pids} ; do
		pid=$(ps h --ppid $parent -o pid)
		if [[ ! -z ${pid} ]] ; then
	         	sudo kill -15 ${pid}
	        fi
	done
        elif [[ ${os} == "darwin"* ]]; then
	# Exit all parent processes
	    for pid in ${pids} ; do
		#echo ${pid}
		sudo kill -15 ${pid}
            done
        fi
else                              
	echo "No running processes detected"
fi

# Check for running Docker instances(start,rerun,sleep,stop)
containers=$(docker ps -q)
if [[ ! -z "${containers}" ]] ; then
	if $start || $stop ; then
                echo "Cleaning up Docker containers..."

		$(docker-compose down)
	elif $sleep ; then
	        echo "Night night. Zzzzzz..."

	        exit 0 
	fi
else
        echo "No Docker containers running"

	if $rerun ; then
	        echo "Can't rerun without them. Exiting..."

	        exit 1
	elif $sleep ; then
		echo "No need to sleep. Exiting..."

		exit 1
	fi
fi

# Exit early if stopping
if $stop ; then
	echo "Shutting down. Bye Bye."

	exit 0
fi

# Boot up new instances if fresh run
if $start ; then
        echo "Spinning up new Docker containers..."

	$(docker-compose up -d)
	scripts/docker-ifconfig.sh
fi

# Build commands to spawn a process per server and autostart ExpoDB(start,rerun)
if $start || $rerun ; then
    if $make ; then
        printf "\nCompiling ResilientDB...\n"
        docker exec s1 mkdir -p obj
        docker exec s1 make clean
        docker exec s1 make -j2
        printf "${GREEN}ResilientDB is compiled successfully${NC}\n"
    fi

    if [[ ${os} == "linux-gnu" ]]; then
	echo "Spinning up new processes..."

	for i in $(seq 1 $replicas) ; do
	        gnome-terminal --tab --title "s${i}" --command "docker exec -it s${i} bash -c \
			'./rundb -nid$((i - 1))';bash"
        done

	for i in $(seq 1 $clients) ; do
	        gnome-terminal --tab --title "c${i}" --command "docker exec -it c${i} bash -c \
			'./runcl -nid$((i + replicas - 1))';bash"
        done

    elif [[ ${os} == "darwin"* ]]; then
	#echo "I'm a Mac User!"

        for i in $(seq 1 $replicas) ; do
	        osascript <<EOD
                  tell application "Terminal"
                    activate
                    tell application "System Events" to keystroke "t" using command down
                    tell application "Terminal" to set custom title of tab 1 of front window to "s${i}"
                    tell application "Terminal" to do script "docker exec -it s${i} bash -c \
		      './rundb -nid$((i - 1))';bash" in front window
                  end tell 
EOD
	done

	for i in $(seq 1 $clients) ; do
	        osascript << EOD
                  tell application "Terminal"
                    activate
                    tell application "System Events" to keystroke "t" using command down
                    tell application "Terminal" to set custom title of tab 1 of front window to "c${i}"
                    tell application "Terminal" to do script "docker exec -it c${i} bash -c \
		      './runcl -nid$((i + replicas - 1))';bash" in front window
                  end tell 
EOD
        done
    fi
fi
