#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

#Check if docker command exists
if ! hash docker 2>/dev/null; then
    echo "Docker command not found. Please install Docker"
    exit 1
fi

if [ "$(uname)" == "Darwin" ]; then
	flags="E"
else
	flags="P"
fi

file="ifconfig.txt"

if [ -f $file ]; then
    echo "ifconfig file exists... Deleting File"
    rm $file
    printf "${GREEN}Deleted${NC}\n"
fi

#Command to get just the names of all the servers setup in docker
OUTPUT="$(docker ps --format '{{.Names}}' | grep -${flags} "c[\d]*|s[\d]*" | sort)"
#echo "${OUTPUT}"
IPC=""
echo "Server sequence --> IP"
CLIENTS=""
for server in ${OUTPUT}; do
    #Get just the IP Address of all the servers
    #echo "$server"
    if ! [ "$server" = "${server#c}" ]; then
        IPC="$(docker inspect $server | grep -${flags}o '"IPAddress": *"([0-9]{1,3}[\.]){3}[0-9]{1,3}"'| grep -${flags}o "([0-9]{1,3}[\.]){3}[0-9]{1,3}")"
        if [ -z "$CLIENTS" ]; then
            CLIENTS="${IPC}"
        else
            CLIENTS="${CLIENTS}\n${IPC}"
        fi
        echo "$server --> ${IPC}"
    else
        IP="$(docker inspect $server | grep -${flags}o '"IPAddress": *"([0-9]{1,3}[\.]){3}[0-9]{1,3}"'| grep -${flags}o "([0-9]{1,3}[\.]){3}[0-9]{1,3}")"
        echo "$server --> ${IP}"
        #Append in ifconfig,txt
        echo "${IP}" >>$file
    fi
done
echo "Put Client IP at the bottom"
echo -e $CLIENTS >>$file
printf "${GREEN}$file Created!${NC}\n"
