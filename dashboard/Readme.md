### Steps to run dashboard

1. Install Nginx and use ```nginx-site.conf``` as your server configuration
2. Install Docker CE, Docker-compose, nodejs, npm
3. Untar ```dashboard.tgz```
4. Do ```npm install``` in "front" and "backend" directories
5. Do ```npm build``` in "front" directory
6. In the "influx" directory do ```docker-compose up -d```
7. From ```scripts/simRun.py``` change ```dashboard``` variable to 'True'
8. Add your client machine IP addresses to ```backend/4_core_ips.txt```
9. Add your server machine IP addresses to ```backend/8_core_ips.txt```
10. Install ```inotify-tools``` on your machines
