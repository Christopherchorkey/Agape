from coherence_engine import BayesianClassifier, InteractionLogger, get_response
from coherence_engine.utils import load_json_data
import json
import math

def main():
    # Load training data
    train_data = load_json_data("train_data.json")

    # Initialize models
    bayesian = BayesianClassifier()
    bayesian.train(train_data["bayesian"])

    # Initialize logger
    logger = InteractionLogger("interactions.json")

    print("=== Coherence Engine CLI (Dictionary Responses) ===")
    print("Type 'quit' to exit.\n")

    while True:
        user_input = input("> ").strip()
        if user_input.lower() == "quit":
            break

        # Bayesian intent detection
        bayesian_explanation = bayesian.explain(user_input)
        predicted_intent = bayesian_explanation["predicted_label"]
        confidence = bayesian_explanation["probabilities"][predicted_intent]

        # Calculate entropy
        probabilities = bayesian_explanation["probabilities"]
        entropy = -sum(
            p * math.log(p) if p > 0 else 0
            for p in probabilities.values()
        )

        # Get dictionary-based response
        response_data = get_response(
            intent=predicted_intent,
            confidence=confidence,
            explanation=json.dumps(bayesian_explanation["probabilities"])
        )

        # Combine outputs
        system_output = {
            "intent": predicted_intent,
            "response": response_data["response"],
            "confidence": confidence,
            "entropy": entropy,
            "response_explanation": response_data["explanation"]
        }

        # Log interaction
        logger.log_interaction(
            user_input=user_input,
            system_output=system_output,
            metadata={
                "bayesian": bayesian_explanation,
                "response_data": response_data
            }
        )

        # Display response
        print("\n--- Coherence Engine Response ---")
        print(f"Intent: {predicted_intent} (Confidence: {confidence:.2%})")
        print(f"Entropy (Uncertainty): {entropy:.2f}")
        print(f"Response: {response_data['response']}")
        print(f"Explanation: {response_data['explanation']}")
        print("---------------------------------\n")

if __name__ == "__main__":
    main()