import json

def load_json_data(filepath):
    """Load JSON data from a file."""
    with open(filepath, "r") as f:
        return json.load(f)

def save_json_data(data, filepath):
    """Save data to a JSON file."""
    with open(filepath, "w") as f:
        json.dump(data, f, indent=2)