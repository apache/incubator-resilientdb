import re
import pandas as pd

# 1. Define the input and output filenames
input_filename = 'stress_test_results.txt'
output_filename = 'parsed_log_data.csv'

# 2. Initialize a list to store the extracted data
extracted_data = []

# 3. Open and read the file
try:
    with open(input_filename, 'r', encoding='utf-8') as f:
        current_step = None
        
        for line in f:
            line = line.strip()
            
            # Check for the Step line using Regex
            # Pattern looks for "[Step " followed by digits
            step_match = re.search(r'\[Step (\d+)/\d+\]', line)
            if step_match:
                current_step = int(step_match.group(1))
                continue
            
            # Check for the Time line
            # Pattern looks for "Add operation took: " followed by digits and " ms"
            # We skip the emoji check to make it more robust against encoding issues
            if "Add operation took:" in line:
                time_match = re.search(r'Add operation took:\s+(\d+)\s+ms', line)
                if time_match and current_step is not None:
                    duration = int(time_match.group(1))
                    
                    # Append the found pair to our list
                    extracted_data.append({
                        'Step': current_step, 
                        'Duration_ms': duration
                    })
                    
                    # Reset current_step to ensure we don't duplicate if format is broken
                    current_step = None

    # 4. Convert to DataFrame and Save to CSV
    if extracted_data:
        df = pd.DataFrame(extracted_data)
        df.to_csv(output_filename, index=False)
        print(f"Successfully processed {len(df)} entries.")
        print(df.head())
    else:
        print("No matching data found in the file.")

except FileNotFoundError:
    print(f"Error: The file '{input_filename}' was not found.")