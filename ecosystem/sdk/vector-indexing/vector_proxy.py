# vector_proxy.py
from flask import Flask, request, jsonify
import subprocess
import sys
import re
import os

app = Flask(__name__)

# Get the path of the currently running Python interpreter
# (To ensure scripts run within the same environment/venv)
PYTHON_EXE = sys.executable
CWD = os.path.dirname(os.path.abspath(__file__))

def run_script(script_name, args):
    """
    Executes a Python script as a subprocess and captures its output.
    """
    command = [PYTHON_EXE, script_name] + args
    try:
        # Run the script and capture stdout/stderr
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            cwd=CWD
        )
        
        # Check return code (0 means success)
        if result.returncode != 0:
            return False, result.stderr + "\n" + result.stdout
        return True, result.stdout
    except Exception as e:
        return False, str(e)

# --- API Endpoints ---

@app.route('/', methods=['GET'])
def health_check():
    return jsonify({"status": "online", "message": "Vector Indexing Proxy is running"}), 200

@app.route('/add', methods=['POST'])
def add_vector():
    text = request.json.get('text')
    if not text:
        return jsonify({"error": "No text provided"}), 400

    # Command: python vector_add.py --value "text"
    success, output = run_script("vector_add.py", ["--value", text])

    if success:
        return jsonify({"status": "success", "message": "Added successfully", "raw_output": output.strip()})
    else:
        # Handle specific errors like duplicates
        if "already saved" in output:
             return jsonify({"status": "skipped", "message": "Value already exists"})
        return jsonify({"status": "error", "error": output.strip()}), 500

@app.route('/delete', methods=['POST'])
def delete_vector():
    text = request.json.get('text')
    if not text:
        return jsonify({"error": "No text provided"}), 400

    # Command: python vector_delete.py --value "text"
    success, output = run_script("vector_delete.py", ["--value", text])

    if success:
        return jsonify({"status": "success", "message": "Deleted successfully", "raw_output": output.strip()})
    else:
        return jsonify({"status": "error", "error": output.strip()}), 500

@app.route('/search', methods=['POST'])
def search_vector():
    text = request.json.get('value')
    k = str(request.json.get('k', 3)) # Default to top 3 results
    if not text:
        return jsonify({"error": "No text provided"}), 400

    # Command: python vector_get.py --value "text" --k_matches K
    success, output = run_script("vector_get.py", ["--value", text, "--k_matches", k])

    if not success:
        return jsonify({"status": "error", "error": output.strip()}), 500

    # Parse the stdout from vector_get.py to create a JSON response
    # Expected format example: "1. hello world // (similarity score: 0.1234)"
    results = []
    for line in output.splitlines():
        # Regex to extract text and score
        match = re.search(r'^\d+\.\s+(.*?)\s+//\s+\(similarity score:\s+([0-9.]+)\)', line)
        if match:
            results.append({
                "text": match.group(1),
                "score": float(match.group(2))
            })
        # Capture other informational lines if necessary
        elif line.strip() and "Critical Error" not in line:
             pass

    return jsonify({"status": "success", "results": results})

if __name__ == '__main__':
    # Run on port 5000, accessible externally
    app.run(host='0.0.0.0', port=5000)