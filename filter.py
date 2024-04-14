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


import requests
import json

def fix_json_with_commas(mixed_content):
    # Manually parsing the JSON objects by counting braces
    json_objects = []
    brace_count = 0
    current_json = ''
    in_json = False
    
    for char in mixed_content:
        if char == '{':
            if brace_count == 0:
                in_json = True
                current_json = char
            else:
                current_json += char
            brace_count += 1
        elif char == '}':
            brace_count -= 1
            if brace_count == 0 and in_json:
                current_json += char
                json_objects.append(current_json)
                in_json = False
            else:
                current_json += char
        elif in_json:
            current_json += char

    # Combine all JSON blocks into a valid JSON array string
    combined_json = '[' + ','.join(json_objects) + ']'
    return combined_json

def get_json_objects_by_public_key(json_data, owner_public_key=None, recipient_public_key=None):
    matching_objects = []
    for obj in json_data:
        try:
            # Check if the necessary fields are present
            if 'inputs' in obj and 'outputs' in obj:
                if owner_public_key is not None and recipient_public_key is not None:
                    if owner_public_key in obj['inputs'][0]['owners_before'] and recipient_public_key in obj['outputs'][0]['public_keys']:
                        matching_objects.append(obj)
                elif owner_public_key is None and recipient_public_key is not None:
                    if recipient_public_key in obj['outputs'][0]['public_keys']:
                        matching_objects.append(obj)
                elif owner_public_key is not None and recipient_public_key is None:
                    if owner_public_key in obj['inputs'][0]['owners_before']:
                        matching_objects.append(obj)
                else:
                    matching_objects.append(obj)  # Append all if no keys specified
        except Exception as e:
            print(f"Error processing JSON object: {e}")
    return matching_objects

def get_json_data(url, ownerPublicKey=None, recipientPublicKey=None):
    try:
        response = requests.get(url)
        # Check if the request was successful (status code 200)
        if response.status_code == 200:
            # Parse the JSON data from the response
            json_text = fix_json_with_commas(response.text)
            json_data = json.loads(json_text)
            if ownerPublicKey == None and recipientPublicKey == None:
                matching_objects = get_json_objects_by_public_key(json_data, None, None)
                return matching_objects
            elif ownerPublicKey == None and recipientPublicKey != None:
                matching_objects = get_json_objects_by_public_key(json_data, None, recipientPublicKey)
                return matching_objects
            elif ownerPublicKey != None and recipientPublicKey == None:
                matching_objects = get_json_objects_by_public_key(json_data, ownerPublicKey, None)
                return matching_objects
            else:
                # Get all JSON objects that match the given publicKey
                matching_objects = get_json_objects_by_public_key(json_data, ownerPublicKey, recipientPublicKey)
                return matching_objects
        else:
            print(f"Error: Unable to retrieve data from {url}. Status code: {response.status_code}")
            return None

    except requests.exceptions.RequestException as e:
        print(f"Error: {e}")
        return None

def filter_by_keys(url, ownerPublicKey, recipientPublicKey):
    json_data = get_json_data(url, ownerPublicKey, recipientPublicKey)
    return json_data
