# resilientdb-ansible

Docker image to provision and run ResilientDB along with supporting services (GraphQL, Crow HTTP server, Nginx) using systemd and Ansible.

---

## Quick Start

Build the Docker image:

```bash
docker build -t resilientdb-ansible .
```

Run the container:

```bash
docker run --privileged -v /sys/fs/cgroup:/sys/fs/cgroup:ro -p 80:80 -p 18000:18000 -p 8000:8000 resilientdb-ansible
```