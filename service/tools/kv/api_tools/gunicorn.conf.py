#!/usr/bin/env python3
"""
Gunicorn configuration for ResLens Flamegraph Analysis Service
"""

import multiprocessing

# Server socket
bind = "0.0.0.0:8080"
backlog = 2048

# Worker processes
workers = 1  # Single worker for this service
worker_class = "sync"
worker_connections = 1000
max_requests = 1000
max_requests_jitter = 50

# Timeout
timeout = 30
keepalive = 2

# Logging
accesslog = "-"
errorlog = "-"
loglevel = "info"
access_log_format = '%(h)s %(l)s %(u)s %(t)s "%(r)s" %(s)s %(b)s "%(f)s" "%(a)s"'

# Process naming
proc_name = "reslens-flamegraph-service"

# Preload app for better performance
preload_app = True

# Worker timeout
graceful_timeout = 30

# Restart workers after this many requests
max_requests = 1000
max_requests_jitter = 50 