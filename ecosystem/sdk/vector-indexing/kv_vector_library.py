# Typical Python imports
from typing import Any
import requests
import json

url = "http://127.0.0.1:8000/graphql"

def format_get_responses(results: Any) -> None:
    if (len(results) == 0):
        print('No values with embeddings stored in ResDB!')
    else:
        for i, pairing in enumerate(results):
            text = pairing["text"]
            # There's probably a better way of telling if there is a score, but thats okay
            try:
                score = pairing["score"]
                score *= 100
                print(f"{i+1}. {text} // (similarity score: {score:.2f}%)")
            except KeyError:
                print(f"{i+1}. {text}")

# Returns TRUE for success, FALSE otherwise
def add_value(value: str) -> bool:
    query = f"""
    mutation {{
        addVector(text: "{value}")
    }}
    """
    response = requests.post(url, json={"query": query})
    return ((199 < response.status_code) and (response.status_code < 300))

# Returns TRUE for success, FALSE otherwise
def delete_value(value: str) -> bool:
    query = f"""
    mutation {{
        deleteVector(text: "{value}")
    }}
    """
    response = requests.post(url, json={"query": query})
    return ((199 < response.status_code) and (response.status_code < 300))

# Returns TRUE for success, FALSE otherwise
def get_value(value: str, k: int = 1) -> bool:
    query = f"""
    query {{
        searchVector(text: "{value}", k: {k}) {{
            text
            score
        }}
    }}
    """
    response = requests.post(url, json={"query": query})
    responseDestructured = (response.json())["data"]["searchVector"]
    success_response = (199 < response.status_code) and (response.status_code < 300)
    if success_response:
        format_get_responses(responseDestructured)
    return success_response

# Returns TRUE for success, FALSE otherwise
def get_values() -> bool: 
    query = f"""
    query {{
        getAllVectors {{
            text
        }}
    }}
    """
    response = requests.post(url, json={"query": query})
    responseDestructured = (response.json())["data"]["getAllVectors"]
    success_response = (199 < response.status_code) and (response.status_code < 300)
    if success_response:
        format_get_responses(responseDestructured)
    return success_response