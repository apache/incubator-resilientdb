# ResilientDB Random Data HTTP Service

This HTTP service provides REST API endpoints to execute random data operations in ResilientDB, making it easy to integrate with other systems and tools.

## Features

- **REST API**: Execute random data operations via HTTP endpoints
- **Automatic Path Detection**: Automatically finds the ResilientDB project root
- **Background Loop**: Start/stop continuous data generation in the background
- **Real-time Status**: Monitor loop status and results
- **CORS Support**: Cross-origin requests enabled for web applications

## Quick Start

### 1. Install Dependencies

```bash
pip install flask flask-cors
```

### 2. Start the Service

```bash
# Start on default port 8080
python3 platform/statistic/http_random_data_service.py

# Start on custom port
python3 platform/statistic/http_random_data_service.py 9090
```

### 3. Test the Service

```bash
# Health check
curl http://localhost:8080/health

# Get project info
curl http://localhost:8080/info

# Execute test with 5 operations
curl http://localhost:8080/test?count=5

# Execute test with JSON body
curl -X POST http://localhost:8080/test \
  -H "Content-Type: application/json" \
  -d '{"count": 10}'
```

## API Endpoints

### Health Check
```
GET /health
```
Returns service health status.

### Project Information
```
GET /info
```
Returns ResilientDB project configuration information.

### Execute Test
```
GET /test?count=5
POST /test
```
Execute random data operations.

**GET Parameters:**
- `count` (optional): Number of operations to execute (default: 5)

**POST Body:**
```json
{
  "count": 10
}
```

### Loop Control
```
POST /loop/start
POST /loop/stop
GET /loop/status
```

Start/stop continuous background loop and get status.

## Environment Variables

- `RESILIENTDB_ROOT`: Set to override automatic project root detection

## Example Usage

### Using curl

```bash
# Start continuous loop
curl -X POST http://localhost:8080/loop/start

# Check loop status
curl http://localhost:8080/loop/status

# Stop loop
curl -X POST http://localhost:8080/loop/stop

# Execute 20 random operations
curl -X POST http://localhost:8080/test \
  -H "Content-Type: application/json" \
  -d '{"count": 20}'
```

### Using Python

```python
import requests

# Execute test
response = requests.post('http://localhost:8080/test', 
                        json={'count': 5})
print(response.json())

# Start loop
requests.post('http://localhost:8080/loop/start')

# Get status
status = requests.get('http://localhost:8080/loop/status')
print(status.json())
```

### Using JavaScript

```javascript
// Execute test
fetch('http://localhost:8080/test?count=5')
  .then(response => response.json())
  .then(data => console.log(data));

// Start loop
fetch('http://localhost:8080/loop/start', {method: 'POST'})
  .then(response => response.json())
  .then(data => console.log(data));
```

## Troubleshooting

### Path Issues
If the service can't find the ResilientDB tools, set the `RESILIENTDB_ROOT` environment variable:

```bash
export RESILIENTDB_ROOT=/opt/resilientdb
python3 platform/statistic/http_random_data_service.py
```

### Permission Issues
Make sure the script is executable:

```bash
chmod +x platform/statistic/http_random_data_service.py
```

### Port Already in Use
Use a different port:

```bash
python3 platform/statistic/http_random_data_service.py 9090
```

## Integration with Monitoring

This HTTP service can be easily integrated with monitoring systems like:

- **Prometheus**: Use the `/health` endpoint for health checks
- **Grafana**: Create dashboards using the loop status data
- **Kubernetes**: Deploy as a container with health checks
- **Load Balancers**: Use for health monitoring

## Security Notes

- The service runs on all interfaces (`0.0.0.0`) by default
- Consider using a reverse proxy (nginx, Apache) for production
- Add authentication if needed for production use
- The service executes subprocess commands - ensure proper input validation 