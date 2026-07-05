import math
from collections import defaultdict
import json

class BayesianClassifier:
    def __init__(self, alpha=1.0):
        self.alpha = alpha  # Laplace smoothing
        self.class_stats = {}  # {label: {"word_counts": dict, "total_words": int}}
        self.class_priors = {}  # {label: prior_probability}
        self.vocab = set()

    def train(self, data):
        """Train on labeled data: [{"text": str, "label": str}, ...]"""
        class_counts = defaultdict(int)
        word_counts = defaultdict(lambda: defaultdict(int))

        for item in data:
            label = item["label"]
            class_counts[label] += 1
            words = self._preprocess(item["text"])
            for word in words:
                word_counts[label][word] += 1
                self.vocab.add(word)

        # Calculate priors and store stats
        total_docs = len(data)
        for label, count in class_counts.items():
            self.class_priors[label] = count / total_docs
            total_words = sum(word_counts[label].values())
            self.class_stats[label] = {
                "word_counts": dict(word_counts[label]),
                "total_words": total_words
            }

    def predict(self, text):
        """Predict label and return probabilities."""
        words = self._preprocess(text)
        log_probs = {}

        for label in self.class_priors:
            prior = math.log(self.class_priors[label])
            likelihood = 0.0
            total_words = self.class_stats[label]["total_words"]
            vocab_size = len(self.vocab)

            for word in words:
                count = self.class_stats[label]["word_counts"].get(word, 0)
                prob = (count + self.alpha) / (total_words + self.alpha * vocab_size)
                likelihood += math.log(prob)

            log_probs[label] = prior + likelihood

        # Normalize to probabilities
        max_log = max(log_probs.values())
        exp_probs = {k: math.exp(v - max_log) for k, v in log_probs.items()}
        sum_exp = sum(exp_probs.values())
        probabilities = {k: v / sum_exp for k, v in exp_probs.items()}

        return probabilities

    def explain(self, text):
        """Return a detailed explanation of the prediction."""
        probabilities = self.predict(text)
        predicted_label = max(probabilities, key=probabilities.get)

        explanation = {
            "input": text,
            "predicted_label": predicted_label,
            "probabilities": probabilities,
            "math": {
                "priors": self.class_priors,
                "likelihoods": {
                    label: {
                        word: (self.class_stats[label]["word_counts"].get(word, 0) + self.alpha) /
                              (self.class_stats[label]["total_words"] + self.alpha * len(self.vocab))
                        for word in self._preprocess(text)
                    }
                    for label in self.class_priors
                },
                "vocab_size": len(self.vocab),
                "smoothing_alpha": self.alpha
            }
        }
        return explanation

    @staticmethod
    def _preprocess(text):
        """Basic text preprocessing."""
        return text.lower().split()