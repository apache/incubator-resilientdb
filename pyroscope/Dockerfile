# Use the official Node.js image as the base image
FROM node:18-alpine

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy package.json and package-lock.json to install dependencies
COPY package*.json ./

# Install dependencies
RUN npm install

# Install Pyroscope CLI
RUN apk add --no-cache curl && \
    curl -L https://dl.pyroscope.io/release/pyroscope-0.37.1-linux-amd64.tar.gz | tar xz && \
    mv pyroscope /usr/local/bin/pyroscope && \
    pyroscope --version # Verify the installation of Pyroscope CLI

# Copy the rest of the application code
COPY . .

# Expose the Pyroscope server port (default: 4040)
EXPOSE 4040

# Set the default command to run the server or client
CMD ["sh", "-c", "if [ \"$ROLE\" = \"server\" ]; then pyroscope server --log-level info --storage-path /host/pyroscope-data; else node connect_pyroscope.js; fi"]