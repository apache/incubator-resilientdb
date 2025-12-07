import re
import csv
import sys

def parse_stress_test_log(input_file_path, output_csv_path):
    """
    Parses a stress test log file and exports specific metrics to a CSV.
    
    Args:
        input_file_path (str): Path to the input text file.
        output_csv_path (str): Path where the CSV file will be saved.
    """
    
    # List to store the extracted data rows
    data_rows = []
    
    # State variables to keep track of where we are in the file
    current_step = None
    first_similarity = None
    
    # compiled regex patterns for efficiency
    # Pattern to find [Step X/500]
    step_pattern = re.compile(r'\[Step (\d+)/\d+\]')
    
    # Pattern to find the first entry: starts with "1. " and grabs the percentage
    # Note: We look for lines starting strictly with "1." to avoid other entries
    first_entry_pattern = re.compile(r'^1\..*similarity score:\s+([\d\.]+)\%')
    
    # Pattern to find the operation time: looks for "Get operation took: X ms"
    # Handling the potential emoji or whitespace before "Get"
    time_pattern = re.compile(r'Get operation took:\s+(\d+)\s+ms')

    try:
        with open(input_file_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                
                # 1. Check if line marks the start of a new step
                step_match = step_pattern.search(line)
                if step_match:
                    current_step = step_match.group(1)
                    first_similarity = None # Reset for the new step
                    continue

                # 2. Check for the first data entry (only if we are inside a step)
                if current_step is not None and first_similarity is None:
                    entry_match = first_entry_pattern.search(line)
                    if entry_match:
                        first_similarity = entry_match.group(1)
                        continue

                # 3. Check for the timing line, which usually ends the block
                time_match = time_pattern.search(line)
                if time_match and current_step is not None:
                    time_took = time_match.group(1)
                    
                    # We have all three pieces of data, add to our list
                    # We use 'NA' if similarity wasn't found for some reason
                    sim_val = first_similarity if first_similarity else "NA"
                    
                    data_rows.append([current_step, time_took, sim_val])
                    
                    # Reset step to avoid stale data carrying over (optional safety)
                    current_step = None

        # Write the results to a CSV file
        with open(output_csv_path, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            # Write header
            writer.writerow(['Step Number', 'Operation Time (ms)', 'First Entry Similarity (%)'])
            # Write data
            writer.writerows(data_rows)
            
        print(f"Successfully processed {len(data_rows)} steps.")
        print(f"Output saved to: {output_csv_path}")

    except FileNotFoundError:
        print(f"Error: The file '{input_file_path}' was not found.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    # You can change these filenames as needed
    input_filename = 'stress_test_get_results.txt'
    output_filename = 'stress_test_resultsCSV.csv'
    
    # Create a dummy file for demonstration if it doesn't exist
    # (You should replace this with your actual file)
    import os
    if not os.path.exists(input_filename):
        print(f"'{input_filename}' not found. Please ensure your text file is in the same folder.")
    else:
        parse_stress_test_log(input_filename, output_filename)