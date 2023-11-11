FROM --platform=linux/amd64 ubuntu:latest
RUN apt-get update
RUN apt-get install -y curl python3 pkg-config zip g++ zlib1g-dev unzip gnupg
RUN apt-get install -y ca-certificates-java openjdk-11-jdk
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
RUN mv bazel.gpg /etc/apt/trusted.gpg.d/
RUN echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN apt update -y && apt install -y bazel
RUN apt update -y && apt -y full-upgrade
RUN curl -y --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
RUN bazel --version
COPY . /resilientDB-rust-sdk
WORKDIR /resilientDB-rust-sdk