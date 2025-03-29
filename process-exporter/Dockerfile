# Use a lightweight base image
FROM ncabatoff/process-exporter:latest

# Set the working directory inside the container
WORKDIR /config

# Copy the configuration file into the container
COPY config.yml /config/config.yml

# Set the default command to run the process-exporter
CMD ["--procfs", "/host/proc", "--config.path", "/config/config.yml"]