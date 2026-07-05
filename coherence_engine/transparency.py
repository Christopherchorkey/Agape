import json
import os
from datetime import datetime

class InteractionLogger:
    def __init__(self, log_file="interactions.json"):
        self.log_file = log_file
        self._ensure_log_file()

    def _ensure_log_file(self):
        if not os.path.exists(self.log_file):
            with open(self.log_file, "w") as f:
                json.dump([], f)

    def log_interaction(self, user_input, system_output, metadata=None):
        """Log an interaction with full transparency."""
        interaction = {
            "timestamp": datetime.now().isoformat(),
            "user_input": user_input,
            "system_output": system_output,
            "metadata": metadata or {}
        }
        try:
            with open(self.log_file, "r") as f:
                interactions = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            interactions = []

        interactions.append(interaction)

        with open(self.log_file, "w") as f:
            json.dump(interactions, f, indent=2)

    def get_interactions(self):
        """Retrieve all logged interactions."""
        try:
            with open(self.log_file, "r") as f:
                return json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            return []