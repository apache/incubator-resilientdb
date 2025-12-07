import csv
import statistics
import os

def process_batches(input_csv_path, output_csv_path, batch_size=50):
    """
    Reads a CSV with step data, aggregates it into batches, 
    and calculates averages for time and similarity.
    """
    
    print(f"Reading from: {input_csv_path}")
    
    # 1. Read the data
    data = []
    try:
        with open(input_csv_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                # Parse and clean data
                try:
                    step = int(row['Step Number'])
                    time_val = float(row['Operation Time (ms)'])
                    
                    # Handle potential 'NA' or empty strings for similarity
                    sim_str = row['First Entry Similarity (%)']
                    if sim_str and sim_str != 'NA':
                        sim_val = float(sim_str)
                    else:
                        sim_val = None
                        
                    data.append({
                        'step': step,
                        'time': time_val,
                        'similarity': sim_val
                    })
                except ValueError:
                    # Skip malformed lines
                    continue
    except FileNotFoundError:
        print(f"Error: Could not find file '{input_csv_path}'")
        return

    if not data:
        print("No valid data found to process.")
        return

    # 2. Group into batches
    # We use a dictionary where key is the batch index
    batches = {}
    
    for entry in data:
        # Determine batch index (e.g., Step 1-50 is batch 0, 51-100 is batch 1)
        batch_index = (entry['step'] - 1) // batch_size
        
        if batch_index not in batches:
            batches[batch_index] = {'times': [], 'similarities': []}
        
        batches[batch_index]['times'].append(entry['time'])
        if entry['similarity'] is not None:
            batches[batch_index]['similarities'].append(entry['similarity'])

    # 3. Calculate averages and prepare output rows
    output_rows = []
    sorted_batch_indices = sorted(batches.keys())
    
    for idx in sorted_batch_indices:
        batch_data = batches[idx]
        
        # Define the range string (e.g., "1-50")
        start_step = (idx * batch_size) + 1
        end_step = (idx + 1) * batch_size
        range_label = f"{start_step}-{end_step}"
        
        # Calculate Averages
        avg_time = statistics.mean(batch_data['times']) if batch_data['times'] else 0
        
        if batch_data['similarities']:
            avg_sim = statistics.mean(batch_data['similarities'])
        else:
            avg_sim = 0

        output_rows.append({
            'Batch Range': range_label,
            'Avg Operation Time (ms)': round(avg_time, 2),
            'Avg Similarity Score (%)': round(avg_sim, 2)
        })

    # 4. Write to new CSV
    fieldnames = ['Batch Range', 'Avg Operation Time (ms)', 'Avg Similarity Score (%)']
    
    try:
        with open(output_csv_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(output_rows)
        
        print(f"Success! Processed {len(output_rows)} batches.")
        print(f"Averages saved to: {output_csv_path}")
        
    except IOError as e:
        print(f"Error writing to file: {e}")

if __name__ == "__main__":
    # Settings
    input_file = 'stress_test_resultsCSV.csv'      # Must match the output of the previous script
    output_file = 'stress_test_get_interval_averages.csv'
    
    process_batches(input_file, output_file)