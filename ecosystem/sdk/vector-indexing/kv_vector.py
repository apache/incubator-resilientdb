"""
Filename: kv_vector.py
Description: CLI interface to interact with Vector Functions running in the GraphQL proxy
"""
# Typical Python imports
from pathlib import Path
import sys
# ResDB & HNSW imports
from kv_vector_library import add_value, delete_value, get_value, get_values

# Global Variables
WORKING_DIR = Path("./").resolve()

def help_message():
    print("kv_vector.py --help")
    print("To get instructions on use")
    sys.exit()

if __name__ == "__main__":
    # Suggestion to use help if user's input is completely misguided
    if ((len(sys.argv) < 2) or ((sys.argv[1] != "--add") and (sys.argv[1] != "--delete") and (sys.argv[1] != "--get") and (sys.argv[1] != "--getAll") and (sys.argv[1] != "--help")) ):
            print('Invalid formatting of request! Use:')
            help_message()
    
    # help, a flag that instructs users how to use the tool
    if (sys.argv[1] == '--help'):
        print("This is a tool provided to enable client-side interaction with vector indexing")
        print("All vectors added via this tool will have an embedding generated for them, and will be")
        print("stored (embedding and value) in ResDB. It effectively serves as a wrapper around the")
        print("k/v store - adding and removing key/embedding pairs as instructed. All commands listed below:")
        print('-----------------------------------------------------------------------')
        print("kv_vector.py --add <your_string_value>")
        print("     add the string value to ResDB, and generate an embedding for it")
        print("kv_vector.py --delete <your_string_value>")
        print("     delete the string value from ResDB, as well as its embedding")
        print("kv_vector.py --get <your_string_value>, or:")
        print("kv_vector.py --get <your_string_value> --k_matches <your_integer>")
        print("     get the k-closest values to the input string value, using HNSW.")
        print("     if no k is provided, a default of k=1 will be used")
        print("kv_vector.py --getAll")
        print("     retrieve all values that have a correlated embedding")
        print('-----------------------------------------------------------------------')

    if (sys.argv[1] == "--add"):
        if (len(sys.argv) != 3):
            print("Invalid formatting of request! Use:")
            print("kv_vector.py --add <your_string_value>")
            print("To save and generate an embedding for the chosen string. Alternatively, use:")
            help_message()
        else:
            add_value(sys.argv[2])

    if (sys.argv[1] == "--delete"):
        if (len(sys.argv) != 3):
            print("Invalid formatting of request! Use:")
            print("kv_vector.py --delete <your_string_value>")
            print("To delete a value and embedding for the chosen string. Alternatively, use:")
            help_message()
        else:
            delete_value(sys.argv[2])

    if (sys.argv[1] == "--getAll"):
        if (len(sys.argv) != 2):
            print("Invalid formatting of request! Use:")
            print("kv_vector.py --getAll")
            print("To get a list of every VALUE that currently has a generated embedding. Alternatively, use:")
            help_message()
        else:
            get_values()

    if (sys.argv[1] == "--get"):
        if ((len(sys.argv) == 3)):
            get_value(sys.argv[2])
        elif ((len(sys.argv) == 5) and (sys.argv[3] == "--k_matches")):
            try:
                k_matches = int(sys.argv[4])
                get_value(sys.argv[2], k_matches)
            except ValueError:
                print("Invalid formatting of request! k_matches must be an integer")
                sys.exit()
        else:
            print("Invalid formatting of request! Use:")
            print("kv_vector.py --get <your_string_value>, or:")
            print("kv_vector.py --get <your_string_value> --k_matches <your_integer>")
            print("to find the k-most similar vectors to your input. The default case where k_matches isn't provided, 1 is used")
            print("To save and generate an embedding for the chosen string. Alternatively, use:")
            help_message()