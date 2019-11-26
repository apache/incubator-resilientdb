#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

os="${OSTYPE}"

#Check if docker command exists
if ! hash docker 2>/dev/null; then
    echo "Docker command not found. Please install Docker"
    exit 1
fi

file="ifconfig.txt"

if [ -f $file ]; then
    echo "ifconfig file exists... Deleting File"
    rm $file
    printf "${GREEN}Deleted${NC}\n"
fi

#Command to get just the names of all the servers setup in docker
if [[ ${os} == "darwin"* ]] ; then
    OUTPUT="$(docker ps --format '{{.Names}}' | grep -E "c[0-9]+|s[0-9]+" | sort)"
elif [[ ${os} == "linux-gnu" ]] ; then
    OUTPUT="$(docker ps --format '{{.Names}}' | grep -P "c[\d]+|s[\d]+" | sort)"
fi
echo "${OUTPUT}"
IPC=""
echo "Server sequence --> IP"
CLIENTS=""
for server in ${OUTPUT}; do
    #Get just the IP Address of all the servers
    #echo "$server"
    if ! [ "$server" = "${server#c}" ]; then
        if [[ ${os} == "darwin"* ]]; then

            IPC="$(docker inspect $server | grep -E '"IPAddress": "([0-9]{1,3}[\.]){3}[0-9]{1,3}"')"
            IPC="${IPC#*\"IPAddress\": \"}"
            IPC="${IPC%\"*}"
          elif [[ ${os} == "linux-gnu" ]]; then
            IPC="$(docker inspect $server | grep -Po '"IPAddress": *\K"([0-9]{1,3}[\.]){3}[0-9]{1,3}"')"
            IPC="${IPC%\"}"
            IPC="${IPC#\"}"
        fi
        if [ -z "$CLIENTS" ]; then
            CLIENTS="${IPC}"
        else
            CLIENTS="${CLIENTS}\n${IPC}"
        fi

      echo "$server --> ${IPC}"
    else
        if [[ ${os} == "darwin"* ]]; then
            IP="$(docker inspect $server | grep -E '"IPAddress": "([0-9]{1,3}[\.]){3}[0-9]{1,3}"')"
            #Remove quotes
            IP="${IP#*\"IPAddress\": \"}"
            IP="${IP%\"*}"
        elif [[ ${os} == "linux-gnu" ]]; then
            IP="$(docker inspect $server | grep -Po '"IPAddress": *\K"([0-9]{1,3}[\.]){3}[0-9]{1,3}"')"
            IP="${IP%\"}"
            IP="${IP#\"}"
        fi
        echo "$server --> ${IP}"
        #Append in ifconfig,txt
        echo "${IP}" >>$file
    fi
done
echo "Put Client IP at the bottom"
echo -e $CLIENTS >>$file
printf "${GREEN}$file Created!${NC}\n"
