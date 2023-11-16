# Use ARM64 Ubuntu as base image
FROM arm64v8/ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive 

# Install dependencies
RUN apt-get update && apt-get install -y \
    software-properties-common \
    git \
    curl \
    python3-pip

# Install Bazelisk
RUN curl -Lo /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-arm64 \
    && chmod +x /usr/local/bin/bazel

# Install Python 3.10
RUN add-apt-repository ppa:deadsnakes/ppa
RUN apt-get update && apt-get install -y python3.10 python3-venv

# Set up virtual environment
RUN /usr/bin/python3 --version
RUN /usr/bin/python3 -m venv venv
ENV PATH="/app/venv/bin:$PATH" 

# Install pip packages
RUN pip install --upgrade pip
RUN pip install wheel

# Set the working directory
WORKDIR /app

# Copy your project files to the container
COPY . /app

RUN sh service/tools/start_kv_service_sdk.sh

# Expose port
EXPOSE 18000  

# Set healthcheck
HEALTHCHECK CMD curl -f http://localhost:18000/ || exit 1

RUN bazel build service/http_server/crow_service_main

ENTRYPOINT ["./entrypoint.sh"]