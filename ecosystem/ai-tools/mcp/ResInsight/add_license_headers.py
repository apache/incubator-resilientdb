#!/usr/bin/env python3
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
import os
import glob

# Apache License Header for Python files
PYTHON_HEADER = """# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""

def add_header_to_file(filepath, header):
    """Add Apache license header to a file if not already present."""
    try:
        # Try UTF-8 first
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError:
        try:
            # Try with latin-1 as fallback
            with open(filepath, 'r', encoding='latin-1') as f:
                content = f.read()
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            return
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return
    
    # Check if header already exists
    if 'Apache Software Foundation' in content:
        print(f"Header already exists in {filepath}")
        return
    
    # Add header
    try:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(header + content)
        print(f"✓ Added header to {filepath}")
    except Exception as e:
        print(f"Error writing {filepath}: {e}")

def main():
    # Add to all Python files
    py_files = glob.glob('**/*.py', recursive=True)
    
    if not py_files:
        print("No Python files found!")
        return
    
    print(f"Found {len(py_files)} Python files")
    print("-" * 50)
    
    for py_file in py_files:
        if not py_file.startswith('.'):  # Skip hidden files
            add_header_to_file(py_file, PYTHON_HEADER)
    
    print("-" * 50)
    print("Done!")

if __name__ == '__main__':
    main()
