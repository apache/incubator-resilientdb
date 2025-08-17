#!/usr/bin/env python3
"""
ResLens Flamegraph Analysis Service

A simple HTTP service for executing ResilientDB random data operations
as part of the ResLens monitoring and analysis toolkit.

This utility is used mainly to seed data to create a long running data seeding job for flamegroah analysis as these flamegraph processes need to run for more than 30s.
"""

import os
import sys
import json
import subprocess
import random
import threading
import time
from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

class ResLensToolsService:
    def __init__(self):
        self.project_root = self._find_project_root()
        self.seeding_running = False
        self.seeding_thread = None
        self.seeding_results = []
        self.seeding_lock = threading.Lock()
        print(f"ResLens Tools Service - Using project root: {self.project_root}")
    
    def _find_project_root(self):
        """Find the ResilientDB project root directory."""
        resilientdb_root = os.getenv("RESILIENTDB_ROOT")
        if resilientdb_root:
            return resilientdb_root
        
        # Since we're now in service/tools/kv/api_tools/, go up to project root
        current_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.join(current_dir, "../../../../..")
        
        # Verify this is the project root by checking for bazel-bin
        if os.path.exists(os.path.join(project_root, "bazel-bin")):
            return project_root
        
        # Fallback to common paths
        possible_paths = [
            "/opt/resilientdb",
            "/home/ubuntu/resilientdb", 
            "/home/jyu25/nexres",
            os.path.join(os.path.expanduser("~"), "resilientdb")
        ]
        
        for path in possible_paths:
            tool_path = os.path.join(path, "bazel-bin/service/tools/kv/api_tools/kv_service_tools")
            if os.path.exists(tool_path):
                return path
        
        return "/opt/resilientdb"
    
    def start_seeding(self, count):
        """Start data seeding job in background thread."""
        if self.seeding_running:
            return {
                "service": "ResLens Flamegraph Analysis Service",
                "status": "error",
                "message": "Seeding job already running"
            }
        
        self.seeding_running = True
        self.seeding_results.clear()
        
        def seeding_worker():
            for i in range(count):
                if not self.seeding_running:
                    break
                
                key = f"key{random.randint(0, 499)}"
                value = f"value{random.randint(0, 499)}"
                
                cmd = [
                    f"{self.project_root}/bazel-bin/service/tools/kv/api_tools/kv_service_tools",
                    "--config", f"{self.project_root}/service/tools/config/interface/service.config",
                    "--cmd", "set",
                    "--key", key,
                    "--value", value
                ]
                
                try:
                    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
                    output = f"{i+1}/{count} {result.stdout.strip()}"
                    if result.stderr:
                        output += f" (stderr: {result.stderr.strip()})"
                    
                    with self.seeding_lock:
                        self.seeding_results.append(output)
                    
                except subprocess.TimeoutExpired:
                    with self.seeding_lock:
                        self.seeding_results.append(f"{i+1}/{count} Timeout after 30 seconds")
                except Exception as e:
                    with self.seeding_lock:
                        self.seeding_results.append(f"{i+1}/{count} Error: {str(e)}")
                
                time.sleep(0.1) 
            
            self.seeding_running = False
        
        self.seeding_thread = threading.Thread(target=seeding_worker, daemon=True)
        self.seeding_thread.start()
        
        return {
            "service": "ResLens Flamegraph Analysis Service",
            "status": "success",
            "message": f"Started seeding job with {count} operations"
        }
    
    def stop_seeding(self):
        """Stop the data seeding job."""
        if not self.seeding_running:
            return {
                "service": "ResLens Flamegraph Analysis Service",
                "status": "error",
                "message": "No seeding job running"
            }
        
        self.seeding_running = False
        if self.seeding_thread and self.seeding_thread.is_alive():
            self.seeding_thread.join(timeout=5)
        
        return {
            "service": "ResLens Flamegraph Analysis Service",
            "status": "success",
            "message": "Seeding job stopped"
        }
    
    def get_seeding_status(self):
        """Get current seeding job status."""
        with self.seeding_lock:
            return {
                "service": "ResLens Flamegraph Analysis Service",
                "status": "running" if self.seeding_running else "stopped",
                "results_count": len(self.seeding_results),
                "results": self.seeding_results.copy()
            }

# Global service instance
service = ResLensToolsService()

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint."""
    return jsonify({
        "service": "ResLens Flamegraph Analysis Service",
        "status": "ok"
    })

@app.route('/seed', methods=['POST'])
def start_seeding():
    """Start data seeding job."""
    try:
        data = request.get_json()
        if not data:
            return jsonify({
                "service": "ResLens Flamegraph Analysis Service",
                "status": "error",
                "message": "No JSON data provided"
            }), 400
        
        count = data.get('count')
        if count is None:
            return jsonify({
                "service": "ResLens Flamegraph Analysis Service",
                "status": "error",
                "message": "Missing 'count' parameter"
            }), 400
        
        if not isinstance(count, int) or count <= 0:
            return jsonify({
                "service": "ResLens Flamegraph Analysis Service",
                "status": "error",
                "message": "Count must be a positive integer"
            }), 400
        
        return jsonify(service.start_seeding(count))
        
    except Exception as e:
        return jsonify({
            "service": "ResLens Flamegraph Analysis Service",
            "status": "error",
            "message": str(e)
        }), 500

@app.route('/stop', methods=['POST'])
def stop_seeding():
    """Stop data seeding job."""
    return jsonify(service.stop_seeding())

@app.route('/status', methods=['GET'])
def get_status():
    """Get seeding job status."""
    return jsonify(service.get_seeding_status())

@app.route('/', methods=['GET'])
def root():
    """Root endpoint with service information."""
    return jsonify({
        "service": "ResLens Flamegraph Analysis Service",
        "description": "HTTP service for executing ResilientDB random data operations",
        "endpoints": {
            "GET /health": "Health check",
            "POST /seed": "Start data seeding job (JSON body: {\"count\": 5})",
            "POST /stop": "Stop data seeding job",
            "GET /status": "Get seeding job status"
        },
        "example": {
            "POST /seed": {
                "body": {"count": 10},
                "response": "Starts background job to execute 10 random set operations"
            },
            "POST /stop": {
                "body": "{}",
                "response": "Stops the running seeding job"
            }
        }
    })

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    
    print(f"Starting ResLens Flamegraph Analysis Service on port {port}")
    print("Available endpoints:")
    print("  GET  /health - Health check")
    print("  POST /seed - Start data seeding job")
    print("  POST /stop - Stop data seeding job")
    print("  GET  /status - Get seeding job status")
    print("  GET  / - Service information")
    
    app.run(host='0.0.0.0', port=port, debug=False, threaded=True) 