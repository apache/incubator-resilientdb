FROM --platform=linux/amd64 ubuntu:latest
RUN apt-get update
RUN apt-get install -y curl python3 pkg-config zip g++ zlib1g-dev unzip gnupg libssl-dev
RUN apt update -y && apt -y full-upgrade
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"
COPY . /resilientDB-rust-sdk
WORKDIR /resilientDB-rust-sdk