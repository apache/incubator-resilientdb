# Download Prometheus
wget https://github.com/prometheus/prometheus/releases/download/v2.47.0/prometheus-2.47.0.linux-amd64.tar.gz
tar xvf prometheus-2.47.0.linux-amd64.tar.gz
mv prometheus-2.47.0.linux-amd64 prometheus

# Download node_exporter
wget https://github.com/prometheus/node_exporter/releases/download/v1.6.1/node_exporter-1.6.1.linux-amd64.tar.gz
tar xvf node_exporter-1.6.1.linux-amd64.tar.gz

# Download and Install grafana
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
sudo apt update
sudo apt install grafana

# Start Prometheus
cp prometheus.yml prometheus/

#Start grafana
sudo service grafana-server start

#Start Prometheus 
./prometheus

#Start exporter in each node where the service is running
./node_exporter

#Running Prometheus or Exporter in dark
nohup ./prometheus & 2>&1
nohup ./node_exporter & 2>&1

More details:
https://blog.resilientdb.com/2022/12/06/NexResGrafanaDashboardInstallation.html

