# Monitoring Sidecar for MemLens

This repository contains the **Monitoring Sidecar** for the MemLens project. The sidecar is designed to monitor the parent process running on the host using tools like **Pyroscope** and **Process Exporter**. It runs as a Docker container and collects profiling and process-level metrics for real-time performance monitoring.

---

## Features

- **Profiling with Pyroscope**: Captures CPU and memory profiling data for the parent process on the host.
- **Process Monitoring with Process Exporter**: Exposes process-level metrics for Prometheus scraping.
- **Containerized Deployment**: Runs as a Docker container for easy integration with the MemLens project.
- **Host Monitoring**: Monitors the parent process running on the host by sharing the process namespace and filesystem.

---

## Folder Structure

The repository is organized as follows:

/pyroscope Dockerfile # Builds the Pyroscope server and client image connect_pyroscope.js # Script to connect Pyroscope to the parent process pyroscope-data # Directory for storing Pyroscope server data

/process-exporter Dockerfile # Builds the Process Exporter image config.yml # Configuration file for Process Exporter

/middleware Dockerfile # Builds the middleware API service server.js # Main entry point for the middleware service package.json # Node.js dependencies for the middleware service

docker-compose.yml # Orchestrates the Pyroscope and Process Exporter containers README.md # Documentation for the repository


---

## Prerequisites

Before you begin, ensure you have the following installed:

- **Docker**: Required to run the sidecar containers.
- **Prometheus**: For scraping metrics exposed by the Process Exporter.
- **Pyroscope**: For profiling data visualization.

---

## Usage

### 1. Build and Run the Sidecar
Use Docker Compose to build and run the monitoring sidecar:
```bash
docker-compose up --build
```
### 2. Access Pyroscope
```bash
http://localhost:4040
```
### 3. Access Process Exporter Metrics
```bash
http://localhost:9256/metrics
```

---

## How It Works
### Pyroscope Client:

The Pyroscope client connects to the parent process running on the host using the connect_pyroscope.js script.
It uses eBPF or other profiling methods to capture CPU and memory usage.

### Process Exporter:

The Process Exporter monitors the parent process and exposes metrics in a Prometheus-compatible format.
It uses a configuration file (config.yml) to specify which processes to monitor.
Host Integration:

The containers share the host's process namespace (pid: "host") and mount the host's proc filesystem to access process information.

### Security Considerations
The containers run with elevated privileges (privileged: true) to access host resources. Use this setup cautiously in production environments.
Ensure that only trusted users can access the exposed metrics and profiling data.

## TODO
1. Add ResView as a sidecar service.
2. Feature ideas for log tracing on both crow and resdb