#!/usr/bin/env python3
"""Quick script to list available Gemini models for your API key."""
import os
import sys
from google import genai

# Load env vars
from dotenv import load_dotenv
load_dotenv()

api_key = os.getenv("GEMINI_API_KEY")
if not api_key:
    print("Error: GEMINI_API_KEY not set in environment")
    sys.exit(1)

try:
    client = genai.Client(api_key=api_key)
    print("Fetching available models...")
    models = client.models.list()
    
    print("\nAvailable models:")
    print("-" * 60)
    for model in models:
        print(f"  - {model.name}")
        if hasattr(model, 'display_name'):
            print(f"    Display: {model.display_name}")
    
    print("\nTry setting GEMINI_MODEL to one of the model names above.")
except Exception as e:
    print(f"Error listing models: {e}")
    print("\nTrying common model names manually:")
    print("  - gemini-pro")
    print("  - gemini-1.5-pro")
    print("  - gemini-1.5-flash")
