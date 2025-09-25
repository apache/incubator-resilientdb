import matplotlib.pyplot as plt
import numpy as np
import json
import argparse
import sys
from matplotlib.ticker import ScalarFormatter

def format_value(value):
    """Format large numbers with K/M suffix"""
    if value >= 1000000:
        return f'{value/1000000:.1f}M'
    elif value >= 1000:
        return f'{value/1000:.1f}K'
    return f'{value:.1f}'

def calculate_overhead(primary_values, comparison_values):
    """Calculate overhead percentages relative to primary values"""
    return [(comp - prim) / prim * 100 for prim, comp in zip(primary_values, comparison_values)]

def plot_benchmark(data, output_file=None):
    # Extract data
    records = data["data"]["records"]
    series = data["data"]["series"]
    
    # Get primary values for overhead calculation
    primary_values = series[0]["values"]
    
    # Calculate overheads for other series
    overheads = []
    for s in series[1:]:
        overhead = calculate_overhead(primary_values, s["values"])
        overheads.append(overhead)
    
    # Set up positions
    x = np.arange(len(records))
    width = 0.2
    
    # Create figure and axis
    fig, ax = plt.subplots(figsize=(12, 6))
    
    # Create bars
    bars = []
    for i, series_data in enumerate(series):
        values = series_data["values"]
        bar = ax.bar(x + (i - 1.5)*width, 
                    values, 
                    width, 
                    label=series_data["name"], 
                    color=series_data["color"])
        bars.append(bar)
    
    # Add overhead annotations for non-primary series
    for i in range(1, len(series)):
        for j, value in enumerate(series[i]["values"]):
            overhead = overheads[i-1][j]
            x_pos = x[j] + (i - 1.5)*width
            
            # Only show overhead percentage, positioned at top of bar
            if overhead > 0:
                overhead_text = f'+{overhead:.1f}%'
            else:
                overhead_text = f'{overhead:.1f}%'
                
            ax.text(x_pos, value * 1.02,
                   overhead_text,
                   ha='center',
                   va='bottom',
                   fontsize=8,
                   alpha=0.7)
    
    # Customize the plot
    ax.set_yscale('log')
    ax.set_ylabel(data["data"]["axis"]["y"]["label"])
    ax.set_xlabel(data["data"]["axis"]["x"]["label"])
    ax.set_title(data["title"])
    ax.set_xticks(x)
    ax.set_xticklabels(records)
    
    # Format y-axis with K/M suffixes
    ax.yaxis.set_major_formatter(ScalarFormatter())
    ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda x, p: format_value(x)))
    
    ax.legend()
    ax.grid(True, which="both", ls="-", alpha=0.2)
    
    # Add subtle horizontal lines for easier reading
    ax.yaxis.grid(True, linestyle='--', alpha=0.3)
    
    # Adjust layout
    plt.tight_layout()
    
    # Save the plot
    output_file = output_file or 'benchmark_latency_comparison.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Plot saved as: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='Generate benchmark latency comparison plots')
    parser.add_argument('input_file', nargs='?', type=str, 
                      help='Input JSON file containing benchmark data')
    parser.add_argument('--output', '-o', type=str,
                      help='Output PNG file (default: benchmark_latency_comparison.png)')
    parser.add_argument('--example', '-e', action='store_true',
                      help='Print example JSON format and exit')
    parser.add_argument('--dpi', type=int, default=300,
                      help='DPI for output image (default: 300)')
    parser.add_argument('--figsize', type=float, nargs=2, default=[12, 6],
                      help='Figure size in inches (width height), default: 12 6')

    args = parser.parse_args()

    if args.example:
        example_data = {
            "title": "Latency Comparison: 25%, 50%, 100% Write Amplification, Sequential Reads",
            "units": "ms",
            "data": {
                "records": ["1K", "10K", "100K", "1M"],
                "series": [
                    {
                        "name": "Primary Only",
                        "values": [18.8, 206, 2231, 29095],
                        "color": "#2196F3"
                    },
                    {
                        "name": "25% Write Amplification",
                        "values": [19.1, 209, 2233, 34021],
                        "color": "#4CAF50"
                    }
                ],
                "axis": {
                    "x": {"label": "Number of Records", "scale": "linear"},
                    "y": {"label": "Latency (ms)", "scale": "log"}
                }
            }
        }
        print("Example JSON format:")
        print(json.dumps(example_data, indent=2))
        sys.exit(0)

    if args.input_file:
        try:
            with open(args.input_file, 'r') as f:
                data = json.load(f)
        except FileNotFoundError:
            print(f"Error: Input file '{args.input_file}' not found")
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON in input file: {e}")
            sys.exit(1)
    else:
        print("Error: Please provide an input JSON file or use --example to see the format")
        sys.exit(1)

    plot_benchmark(data, args.output)

if __name__ == "__main__":
    main()