#!/usr/bin/env bash

set -euo pipefail

cd /home/ubuntu/resilient-monitoring/ResLens-Middleware

# Stop existing containers (if any)
sudo docker-compose down || true

# Start containers in detached mode
sudo docker-compose up -d



