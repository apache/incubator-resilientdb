# test.py
import requests
from resdb_orm.orm import ResDBORM

db = ResDBORM()

# Create records
data = {"name": "abc", "age": 123}
create_response = db.create(data)
print("Create Response:", create_response)

# Retrieve records
read_response = db.read(create_response)
print("Read Response:", read_response)
print("Data:", read_response["data"])

# Update records
update_response = db.update(create_response, {"name": "def", "age": 456})
print("Update Response:", update_response)
read_response = db.read(create_response)
print("Read Response:", read_response)
print("New Data:", read_response["data"])

# Delete records
delete_response = db.delete(create_response)
print("Delete Response:", delete_response)
read_response = db.read(create_response)

