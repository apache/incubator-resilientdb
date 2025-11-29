#!/bin/bash

input_file=$1
awk 'NR==1 {print; next} {printf "%s", $1; for(i=2;i<=NF;i++) printf " %.10f", $i*1000; print ""}' "$input_file" > "${input_file}.tmp" && mv "${input_file}.tmp" "$input_file"