#!/bin/bash
# Wait a few seconds to let systemd fully boot
sleep 5

echo "Reloading systemd daemon..."
systemctl daemon-reload

echo "Enabling and starting all custom services..."
systemctl enable nginx && systemctl start nginx
systemctl enable crow-http && systemctl start crow-http
systemctl enable graphql && systemctl start graphql
systemctl enable resilientdb-client && systemctl start resilientdb-client
systemctl enable resilientdb-kv@1 && systemctl start resilientdb-kv@1
systemctl enable resilientdb-kv@2 && systemctl start resilientdb-kv@2
systemctl enable resilientdb-kv@3 && systemctl start resilientdb-kv@3
systemctl enable resilientdb-kv@4 && systemctl start resilientdb-kv@4

echo "Custom services started."
