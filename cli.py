from coherence_engine import BayesianClassifier, MarkovChain, InteractionLogger
from coherence_engine.utils import load_json_data
import json
import math

def main():
    # Load training data
    train_data = load_json_data("train_data.json")

    # Initialize models
    bayesian = BayesianClassifier()
    bayesian.train(train_data["bayesian"])

    markov = MarkovChain(order=1)
    markov.train(train_data["markov"])

    # Initialize logger
    logger = InteractionLogger("interactions.json")

    print("=== Coherence Engine CLI ===")
    print("Type 'quit' to exit.\n")

    while True:
        user_input = input("> ").strip()
        if user_input.lower() == "quit":
            break

        # Bayesian intent detection
        bayesian_explanation = bayesian.explain(user_input)
        predicted_intent = bayesian_explanation["predicted_label"]

        # Markov response generation
        markov_explanation = markov.explain(user_input)

        # Calculate entropy
        probabilities = bayesian_explanation["probabilities"]
        entropy = -sum(
            p * math.log(p) if p > 0 else 0
            for p in probabilities.values()
        )

        # Combine outputs
        system_output = {
            "intent": predicted_intent,
            "response": markov_explanation["response"],
            "confidence": bayesian_explanation["probabilities"][predicted_intent],
            "entropy": entropy
        }

        # Log interaction
        logger.log_interaction(
            user_input=user_input,
            system_output=system_output,
            metadata={
                "bayesian": bayesian_explanation,
                "markov": markov_explanation
            }
        )

        # Display response
        print("\n--- Coherence Engine Response ---")
        print(f"Intent: {predicted_intent} (Confidence: {system_output['confidence']:.2%})")
        print(f"Entropy (Uncertainty): {system_output['entropy']:.2f}")
        print(f"Response: {system_output['response']}")
        print("---------------------------------\n")

if __name__ == "__main__":
    main()