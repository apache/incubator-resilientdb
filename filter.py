import requests
import json

def fix_json_with_commas(json_str):
    fixed_json = json_str.replace('}{', '},{')
    return f"[{fixed_json}]"

def get_json_objects_by_public_key(json_data, owner_public_key=None, recipient_public_key=None):
    matching_objects = []
    if owner_public_key != None and recipient_public_key != None:
        for obj in json_data[0]:
            if owner_public_key in obj['inputs'][0]['owners_before'] and recipient_public_key in obj['outputs'][0]['public_keys']:
                matching_objects.append(obj)
    elif owner_public_key == None and recipient_public_key != None:
        for obj in json_data[0]:
            if recipient_public_key in obj['outputs'][0]['public_keys']:
                matching_objects.append(obj)
    elif owner_public_key != None and recipient_public_key == None:
        for obj in json_data[0]:
            if owner_public_key in obj['inputs'][0]['owners_before']:
                matching_objects.append(obj)
    else:
        for obj in json_data[0]:
            matching_objects.append(obj)
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

print(filter_by_keys("http://localhost:18000/v1/transactions", None, None))