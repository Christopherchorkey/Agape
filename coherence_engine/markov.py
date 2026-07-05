import random
from collections import defaultdict

class MarkovChain:
    def __init__(self, order=1):
        self.order = order  # 1 = bigram, 2 = trigram, etc.
        self.chain = defaultdict(lambda: defaultdict(int))
        self.starts = []  # List of starting sequences

    def train(self, texts):
        """Train on a list of strings."""
        for text in texts:
            words = text.lower().split()
            if len(words) < self.order:
                continue
            # Add to starts
            start_seq = tuple(words[:self.order])
            self.starts.append(start_seq)
            # Build transitions
            for i in range(len(words) - self.order):
                current = tuple(words[i:i + self.order])
                next_word = words[i + self.order]
                self.chain[current][next_word] += 1

    def generate(self, max_length=20):
        """Generate a random sequence."""
        if not self.starts:
            return ""

        current = random.choice(self.starts)
        output = list(current)

        for _ in range(max_length - self.order):
            if current not in self.chain or not self.chain[current]:
                break
            next_words = list(self.chain[current].keys())
            weights = list(self.chain[current].values())
            next_word = random.choices(next_words, weights=weights, k=1)[0]
            output.append(next_word)
            current = tuple(output[-self.order:])

        return " ".join(output)

    def explain(self, prompt, max_length=20):
        """Generate a response and explain the transitions."""
        if not self.starts:
            return {"response": "", "explanation": "No training data."}

        current = random.choice(self.starts)
        output = list(current)
        transitions = [{"from": current, "to": None, "probability": 1.0}]

        for _ in range(max_length - self.order):
            if current not in self.chain or not self.chain[current]:
                break
            next_words = list(self.chain[current].keys())
            weights = list(self.chain[current].values())
            total = sum(weights)
            probabilities = {word: count / total for word, count in zip(next_words, weights)}
            next_word = random.choices(next_words, weights=weights, k=1)[0]

            transitions.append({
                "from": current,
                "to": next_word,
                "probability": probabilities[next_word],
                "options": probabilities
            })
            output.append(next_word)
            current = tuple(output[-self.order:])

        return {
            "response": " ".join(output),
            "transitions": transitions,
            "order": self.order
        }