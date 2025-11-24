find . -type f -name '*sh' -exec sed -i 's/\r//g' {} \;
sudo sh INSTALL.sh