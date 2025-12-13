import sys
import os

def create_file(size_mb):
    # 1. Get the directory where this script is currently living
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    filename = f"val_{size_mb}mb.txt"
    
    # 2. Combine that directory with the filename
    full_path = os.path.join(script_dir, filename)
    
    # Calculate size in bytes
    size_bytes = size_mb * 1024 * 1024
    
    print(f"Generating {full_path}...")
    
    # 3. Write the random data
    with open(full_path, "wb") as f:
        # Note: For extremely large files (e.g. > RAM size), 
        # you might want to write in chunks. 
        # For typical testing (up to a few GB), this is fine.
        f.write(os.urandom(size_bytes))
        
    print(f"Done! Created {full_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 gen_files.py <size_in_mb>")
        sys.exit(1)
        
    create_file(int(sys.argv[1]))