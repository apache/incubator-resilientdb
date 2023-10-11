FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

# Update package lists and install required dependencies
RUN apt-get update && apt-get install -y \
    apt-transport-https \
    curl \
    gnupg \
    protobuf-compiler \
    rapidjson-dev \
    build-essential \
    openjdk-11-jdk \
    zip \
    unzip \
    clang-format

# Install Bazel
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg \
    && mv bazel.gpg /etc/apt/trusted.gpg.d/ \
    && echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list \
    && curl https://bazel.build/bazel-release.pub.gpg | apt-key add - \
    && apt-get update && apt-get install -y bazel=5.0.0 \
    && rm /etc/apt/sources.list.d/bazel.list

# Set the working directory
WORKDIR /app

# Copy your project files to the container
COPY . /app

# Build the project with Bazel 
RUN bazel --version
RUN bazel build @com_github_bazelbuild_buildtools//buildifier:buildifier
RUN bazel build service/tools/kv/api_tools/kv_service_tools
