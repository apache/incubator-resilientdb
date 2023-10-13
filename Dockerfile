FROM ubuntu:20.04

# Prevents prompts and dialogues during package installation
ENV DEBIAN_FRONTEND=noninteractive 

# Setup the deadsnakes PPA and other basic dependencies
RUN apt-get update && apt-get install -y \
    software-properties-common \
    apt-transport-https \
    curl \
    gnupg \
    git \
    && add-apt-repository ppa:deadsnakes/ppa \
    && apt-get update

# Install other required packages
RUN apt-get install -y \
    protobuf-compiler \
    rapidjson-dev \
    clang-format \
    build-essential \
    openjdk-11-jdk \
    zip unzip \
    python3.10 \
    python3.10-dev \
    python3-venv \
    python3-dev \
    python3-pip \
    python3-distutils

# Set python3.10 as the default python3 version
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.10 1 && \
    python3 --version

# Install pip3 using the get-pip.py script
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py && \
    python3 get-pip.py && \
    rm get-pip.py

# Cleanup apt cache
RUN rm -rf /var/lib/apt/lists/*

# Add Bazel repository and its GPG key
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg && \
    mv bazel.gpg /etc/apt/trusted.gpg.d/ && \
    echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" > /etc/apt/sources.list.d/bazel.list && \
    apt-get update && apt-get install -y bazel=5.0.0

# Clone ResilientDB repo
RUN git clone https://github.com/resilientdb/resilientdb.git

# Setup ResilientDB
WORKDIR resilientdb

# Set up pre-push hook
RUN if [ -f ".git/hooks/pre-push" ]; then rm .git/hooks/pre-push; fi && \
    ln -s $PWD/hooks/pre-push $PWD/.git/hooks/pre-push

# Bazel version check and alternative installation if necessary
RUN if ! bazel --version; then \
        wget https://releases.bazel.build/6.0.0/release/bazel-6.0.0-dist.zip && \
        mkdir -p bazel_build && \
        mv bazel-6.0.0-dist.zip bazel_build/ && \
        cd bazel_build && \
        unzip bazel-6.0.0-dist.zip && \
        export JAVA_HOME='/usr/lib/jvm/java-1.11.0-openjdk-arm64/' && \
        env EXTRA_BAZEL_ARGS="--host_javabase=@local_jdk//:jdk" bash ./compile.sh && \
        cp output/bazel /usr/local/bin/ && \
        cd .. && rm -rf bazel_build; \
    fi

# Install buildifier
RUN bazel build @com_github_bazelbuild_buildtools//buildifier:buildifier
RUN bazel build service/tools/kv/api_tools/kv_service_tools

# Clone ResilientDB GraphQL repo
WORKDIR /
RUN git clone https://github.com/ResilientApp/ResilientDB-GraphQL.git

# Install Crow HTTP server dependencies
WORKDIR ResilientDB-GraphQL
RUN bazel build service/http_server:crow_service_main

# Directly install Python SDK dependencies without using venv
RUN pip3 install virtualenv && \
    virtualenv venv && \
    . venv/bin/activate && \
    pip install --upgrade setuptools && \
    pip install -r requirements.txt gunicorn

# After installing all the other requirements
RUN apt-get install -y nginx
CMD ["nginx", "-g", "daemon off;"]
