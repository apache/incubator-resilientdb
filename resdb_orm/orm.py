# resdb_orm/orm.py
import requests
import json
from resdb_driver import Resdb
from resdb_driver.crypto import generate_keypair
import secrets

class ResDBORM:
    def __init__(self, conn_string):
        self.db_root_url = conn_string
        self.db = Resdb(conn_string)

    def generate_token(self, length=64):
        return secrets.token_hex(length // 2)

    def create(self, data):
        token = self.generate_token()
        payload = {"id":token, "data":data}
        headers = {'Content-Type': 'application/json'}
        response = requests.post(f'{self.db_root_url}/v1/transactions/commit',
                             data=json.dumps(payload), headers=headers)

        
        # Check if response is successful and handle empty response content
        if response.status_code == 201:
            if response.content:
                decoded_content = response.content.decode('utf-8')
                id_value = decoded_content.split(': ')[1].strip()
                return id_value
            else:
                return {"status": "create unsuccessful, no content in response"}
    
    def read_all(self):
        response = requests.get(f'{self.db_root_url}/v1/transactions')
        return response.json()

    def read(self, key):
        response = requests.get(f'{self.db_root_url}/v1/transactions/{key}')
        return response.json()

    def delete(self, key):
        payload = {"id": key}
        headers = {'Content-Type': 'application/json'}
        response = requests.post(f'{self.db_root_url}/v1/transactions/commit', 
                     data=json.dumps(payload), headers=headers)
    
        # Check if response is successful and handle empty response content
        if response.status_code == 201:
            if response.content:
                return {"status": "delete successful"} 
            else:
                return {"status": "delete unsuccessful, no content in response"}
    
    def update(self, key, new_data):
        # Delete the existing record first
        delete_response = self.delete(key)
    
        # Handle the response accordingly
        if "status" in delete_response and "no content in response" in delete_response["status"]:
            print("Warning: Delete operation returned no content.")
    
        # Update by creating a new entry with the same key
        payload = {"id": key, "data": new_data}
        headers = {'Content-Type': 'application/json'}
        response = requests.post(f'{self.db_root_url}/v1/transactions/commit', 
                             data=json.dumps(payload), headers=headers)
    
        # Check if response is successful and handle empty response content
        if response.status_code == 201:
            if response.content:
                return {"status": "update successful"}
            else:
                return {"status": "update unsuccessful, no content in response"}

def connect(conn_string):
    return ResDBORM(conn_string)