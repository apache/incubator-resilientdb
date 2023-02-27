# Introduction
Nexres dynamic dashboard is a Grafana base dashboard for Nexres. It aims to provide a simple real-time interface for developers to monitor and diagnose Nexres. The data is stored in the Prometheus time-series database and queried by Grafana using PromeQL. The system usage data is provided by Prometheus third-party exporter Node Exporter.

# Requirements
Using Nexres dynamic dashboard requires the installation of Prometheus, Node Exporter, Prometheus-cpp, and Grafana. It has been successfully tested with Bazel building in C++17 on Ubuntu 20.04.4 LTS (Windows 11 WSL) and Visual Studio Code.

# Installation
## Install Prometheus
go to https://prometheus.io/download/ download prometheus
```
tar xvfz (your-prometheus-tar-file)
```

## Install node_exporter
go to https://github.com/prometheus/node_exporter download the lastest version of node_exporter from release
```
tar xvfz (your-node-exporter-tar-file)
```

## Install grafana
```
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
sudo apt update 
sudo apt install grafana
```

## Change endpoint's server address
Edit [example/start_kv_server.sh](/example/start_kv_server.sh), set the new endpoint address in command line arguments.

# Start the dashboard
Prometheus endpoint start automaticly with nexres. To start nexres:
```
sh example/kv_server.sh
```

## Prometheus
Default prometheus port is 9090
```
./prometheus
```

## Node_exporter
Default node exporter port is 9100
```
./node_exporter
```

## Grafana
Default grafana port is 3000
```
sudo service grafana-server start
sudo service grafana-server enable
```
*Stop grafana: sudo service grafana-server stop*

# Configurate prometheus
## Add prometheus endpoint:
Edit “prometheus.yml” under your prometheus folder
add the following code after “scrape_configs”
```
- job_name: "your_job_name"
    static_configs:
      - targets: ["your_ip_address:port"]
```
Here is the current “scrape_configs” structure, you can find this yml file at [here](/documents/file/prometheus.yml).

![yam_config](/documents/image/dashboard/yml_config.png)

After setup your endpoint information, restart prometheus.

## Change prometheus scraping interval:
change scrape_interval and evaluation_interval.

![yml_interval](/documents/image/dashboard/yml_interval.png)

## Prometheus target:
To check your endpoint status. Start prometheus, go to localhost:9090, and click Targets under Status menu.

![target](/documents/image/dashboard/target.png)

This page shows all your endpoints' status. If the endpoint does not show up on this page, remember to set up your endpoint information in prometheus.yml and restart the database.

![target_list](/documents/image/dashboard/target%20list.png)

# Setup grafana:
## Prometheus connection
Please follow this link to set up the connection between grafana and prometheus. 
> https://prometheus.io/docs/visualization/grafana/

## Import Nexres dynamic dashboard. 
Locate the import button under create menu.

![import](/documents/image/dashboard/import.png)

Download the grafana json from this [link](/documents/file/Nexres-1654906717062.json) and import to grafana.

## Change plot 
Go to grafana configuration and click data source. Select your prometheus database and click to change setting, locate "Scrape interval" and change it to 5s.

![resolution](/documents/image/dashboard/resolution.png)

# Prometheus metrics developemt:
All the prometheus endpoint code and metrics builder are located under Stats class. There are three types of metric variables you can utilize according to the demand. They are Counter, Gauge, and Histogram. You can discover their using scenario in this [link](https://prometheus.io/docs/concepts/metric_types/). 

To properly integrate these metric types to the code, check prometheus-cpp documentation ([Documentation](https://jupp0r.github.io/prometheus-cpp/), [Example](https://github.com/jupp0r/prometheus-cpp)). Here is an example of starting a prometheus endpoint and adding a metric to the database.

Create a server running on port 8080
```
Exposer exposer{"127.0.0.1:8080"};
```

Create a metrics registry (make sure it is alway alive)
```
auto registry = std::make_shared<Registry>();
```

Add a new gauge family to the registry (you can change this part to other metric type as your needed)
```
auto& packet_counter = BuildGauge()
                        .Name("observed_packets_total")
                        .Help("Number of observe packets")
                        .Register(*registry);
```

Add and remember dimensional data
```
auto& tcp_rx_gauge = packet_counter.Add({{"protocol", "tcp"}}, {{"direction", "rx"}});
```

Ask the exposer to scrape the registry on incoming HTTP requests
```
exposer.RegisterCollectable(registry);
```

Set numerical value to gauge variable
```
tcp_rx_gauge.Set(123);
```

To setup these variables in header file, following the code structure in [Stats.h](/statistic/stats.h).

# Testing
## Set random data into nexres
```
bazel run statistic/set_random_data <test/loop> <value>
```

## Bazel build
Run this command at nexres directory
```
bazel build ...
```

## Bazel test
Run this command at nexres directory
```
bazel test ...
```

# Future works
- Research and discover more performance metrics into the dashboard.
- Propose a script that could automatically fetch all servers' addresses and generate prometheus.yml file to serve nexres instances on cloud.
- Establish a safe connection to prometheus endpoint by using reverse proxy through Nginx.
- Explore grafana technique for better visualization
- Integrate dashboard into nexres deployment portal page.

