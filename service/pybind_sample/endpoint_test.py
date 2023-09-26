import requests

print('Example of calling NexRes endpoint from Python')

url = 'http://localhost:8000/v1/transactions'

response = requests.get(url)
print(response)
print(response.content)
