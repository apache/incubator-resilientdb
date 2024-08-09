# resdb_orm/orm.py
import requests
import json
from resdb_driver import Resdb
from resdb_driver.crypto import generate_keypair

class ResDBORM:
    def __init__(self, conn_string):
        self.db_root_url = conn_string
        self.db = Resdb(conn_string)

    def create(self, data):
        keypair = generate_keypair()
        prepared_tx = self.db.transactions.prepare(
            operation="CREATE",
            signers=keypair.public_key,
            asset={"data": data},
        )
        fulfilled_tx = self.db.transactions.fulfill(prepared_tx, private_keys=keypair.private_key)
        response = self.db.transactions.send_commit(fulfilled_tx)

        alphanumeric_id = response.split(":")[1].strip()
        return alphanumeric_id

    def read(self, key):
        response = requests.get(f'{self.db_root_url}/v1/transactions/{key}')
        return response.json()

    def delete(self, key):
        payload = {"id": key}
        print("cp1")
        headers = {'Content-Type': 'application/json'}
        response = requests.post(f'{self.db_root_url}/v1/transactions/commit', 
                     data=json.dumps(payload), headers=headers)
    
        print("cp2")
        # Check if response is successful and handle empty response content
        if response.status_code == 201:
            if response.content:
                return {"status": "delete successful"} 
            else:
                return {"status": "delete successful, no content in response"}
    
    def update(self, key, new_data):
        # Delete the existing record first
        delete_response = self.delete(key)
    
        # Handle the response accordingly
        if "status" in delete_response and "no content in response" in delete_response["status"]:
            print("Warning: Delete operation returned no content.")
    
        # Update by creating a new entry with the same key
        payload = {"id": key, "value": new_data}
        headers = {'Content-Type': 'application/json'}
        response = requests.post(f'{self.db_root_url}/v1/transactions/commit', 
                             data=json.dumps(payload), headers=headers)
    
        # Check if response is successful and handle empty response content
        if response.status_code == 201:
            if response.content:
                return {"status": "update successful"}
            else:
                return {"status": "update successful, no content in response"}

def connect(conn_string):
    return ResDBORM(conn_string)
