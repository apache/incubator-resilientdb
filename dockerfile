# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Let systemd know we're in a container
ENV container docker
ENV DEBIAN_FRONTEND=noninteractive

# Update apt and install required packages including systemd, ansible, etc.
RUN apt-get update && \
    apt-get install -y gnupg curl systemd ansible sudo git && \
    # Add Bazel's public key
    curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > /etc/apt/trusted.gpg.d/bazel.gpg && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Set up passwordless sudo (container runs as root)
RUN echo "root ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/99_nopasswd && \
    chmod 0440 /etc/sudoers.d/99_nopasswd

# Copy the ansible playbook project into the container
COPY . /opt/resilientdb-ansible
WORKDIR /opt/resilientdb-ansible

# Run the ansible playbook non-interactively (passwordless sudo)
RUN ansible-playbook site.yml -i inventories/production/hosts --tags all -e "bazel_jobs=1"

# Copy the startup script and unit file into the container
COPY startup.sh /opt/resilientdb-ansible/startup.sh
RUN chmod +x /opt/resilientdb-ansible/startup.sh

# Copy the startup unit file and enable it
COPY startup-services.service /etc/systemd/system/startup-services.service

# Enable the startup service (so that it runs on boot)
RUN systemctl enable startup-services.service || true

# Expose required ports
EXPOSE 80 18000 8000

# Start systemd as PID1
CMD ["/sbin/init"]
