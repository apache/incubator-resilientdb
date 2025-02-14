# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Let systemd know we're in a container
ENV container docker
ENV DEBIAN_FRONTEND=noninteractive

# Update apt and install required packages including gnupg
RUN apt-get update && \
    apt-get install -y gnupg curl systemd ansible sudo git && \
    # Add Bazel's public key (using gpg --dearmor so it can be placed in trusted.gpg.d)
    curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > /etc/apt/trusted.gpg.d/bazel.gpg && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set up passwordless sudo (container runs as root so this is just in case)
RUN echo "root ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/99_nopasswd && \
    chmod 0440 /etc/sudoers.d/99_nopasswd

# Copy the ansible playbook project into the container
COPY . /opt/resilientdb-ansible
WORKDIR /opt/resilientdb-ansible

# Run the ansible playbook non-interactively (no need for -K since we run as root)
RUN ansible-playbook site.yml -i inventories/production/hosts --tags all -e "bazel_jobs=1"

# Expose ports: 80 for Nginx, 18000 for Crow, 8000 for GraphQL
EXPOSE 80 18000 8000

# Use systemd as the container's init system
CMD ["/sbin/init"]