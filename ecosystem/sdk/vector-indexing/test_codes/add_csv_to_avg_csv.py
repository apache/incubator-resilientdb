import csv
from statistics import mean

# 1. Define file names
input_file = 'parsed_log_data.csv'       # Input file
output_file = 'averaged_intervals.csv' # Output file
interval_size = 50

# Data structure to hold durations for each batch
# Format: { batch_id: [duration1, duration2, ...] }
batches = {}

try:
    print("Processing...")
    with open(input_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        
        for row in reader:
            # Convert strings to integers
            try:
                step = int(row['Step'])
                duration = int(row['Duration_ms'])
            except ValueError:
                continue # Skip bad rows
            
            # Calculate which batch this step belongs to
            # Steps 1-50 -> Batch 0, Steps 51-100 -> Batch 1
            batch_id = (step - 1) // interval_size
            
            if batch_id not in batches:
                batches[batch_id] = []
            
            batches[batch_id].append(duration)

    # Write the results
    with open(output_file, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        # Write Header
        writer.writerow(['Start_Step', 'End_Step', 'Average_Duration_ms'])
        
        # Sort by batch_id so the output is in order (1-50, 51-100...)
        sorted_batch_ids = sorted(batches.keys())
        
        for b_id in sorted_batch_ids:
            durations = batches[b_id]
            
            # Calculate Average
            avg_val = mean(durations)
            
            # Calculate Start/End labels for clarity
            start_step = (b_id * interval_size) + 1
            end_step = (b_id + 1) * interval_size
            
            writer.writerow([start_step, end_step, f"{avg_val:.2f}"])

    print(f"Success! Output saved to {output_file}")

except FileNotFoundError:
    print(f"Error: Could not find {input_file}")
except Exception as e:
    print(f"An error occurred: {e}")