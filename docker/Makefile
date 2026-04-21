.PHONY: help start stop restart logs status benchmark metrics clean baseline tiered

RESDB_IMAGE ?= resilientdb:latest
IPFS_IMAGE ?= ipfs/kubo:latest

help:
	@echo "ResilientDB Docker Cluster Management"
	@echo ""
	@echo "Usage:"
	@echo "  make start                Start 4-node cluster (LEVELDB_ONLY mode)"
	@echo "  make baseline             Same as make start"
	@echo "  make tiered              Start 4-node cluster with tiered storage"
	@echo "  make stop                Stop all containers"
	@echo "  make restart            Restart cluster"
	@echo "  make logs               Show logs"
	@echo "  make status            Show container status"
	@echo "  make benchmark        Run benchmark (TPS=100, 60s)"
	@echo "  make metrics          Collect metrics"
	@echo "  make clean             Clean up containers and volumes"
	@echo ""
	@echo "Environment Variables:"
	@echo "  RESDB_IMAGE           ResilientDB image (default: resilientdb:latest)"
	@echo "  IPFS_IMAGE           IPFS image (default: ipfs/kubo:latest)"
	@echo "  TPS                  Benchmark TPS (default: 100)"
	@echo "  DURATION             Benchmark duration in seconds (default: 60)"
	@echo ""
	@echo "Examples:"
	@echo "  make start RESDB_IMAGE=myregistry/resilientdb:v1.0"
	@echo "  make tiered COLD_THRESHOLD=3"
	@echo "  make benchmark TPS=500 DURATION=120"

start baseline:
	@echo "Starting cluster in baseline mode..."
	STORAGE_BACKEND=LEVELDB_ONLY TIERED_ENABLED=false docker-compose up -d
	@echo "Cluster started. Use 'make logs' to view logs."

tiered:
	@echo "Starting cluster with tiered storage..."
	STORAGE_BACKEND=TIERED TIERED_ENABLED=true docker-compose up -d
	@echo "Cluster started with tiered storage. Use 'make logs' to view logs."

stop:
	@echo "Stopping cluster..."
	docker-compose down

restart: stop start

logs:
	docker-compose logs -f

status:
	docker-compose ps

benchmark:
	@./run_benchmark.sh $(TPS) $(DURATION) 1024 localhost 18001

metrics:
	@./collect_metrics.sh metrics.json 4

clean:
	@echo "Cleaning up..."
	docker-compose down -v
	@echo "Done. Use 'make start' to restart."

validate-config:
	@docker-compose config > /dev/null && echo "Config is valid" || echo "Config is invalid"