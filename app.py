from flask import Flask, request, jsonify
from coherence_engine import BayesianClassifier, InteractionLogger, get_response
from coherence_engine.utils import load_json_data
import math
import json

app = Flask(__name__)

# Load models and data
train_data = load_json_data("train_data.json")
bayesian = BayesianClassifier()
bayesian.train(train_data["bayesian"])
logger = InteractionLogger("interactions.json")

@app.route("/interact", methods=["POST"])
def interact():
    data = request.json
    user_input = data.get("input", "")

    # Bayesian intent detection
    bayesian_explanation = bayesian.explain(user_input)
    predicted_intent = bayesian_explanation["predicted_label"]
    confidence = bayesian_explanation["probabilities"][predicted_intent]

    # Calculate entropy
    probabilities = bayesian_explanation["probabilities"]
    entropy = -sum(p * math.log(p) if p > 0 else 0 for p in probabilities.values())

    # Get resonant response
    response_data = get_response(
        intent=predicted_intent,
        confidence=confidence,
        explanation=json.dumps(probabilities),
        user_input=user_input
    )

    # Prepare response
    system_output = {
        "intent": predicted_intent,
        "response": response_data["response"],
        "confidence": confidence,
        "entropy": entropy,
        "explanation": {
            "bayesian": bayesian_explanation,
            "response_explanation": response_data["explanation"]
        }
    }

    # Log interaction
    logger.log_interaction(
        user_input=user_input,
        system_output=system_output,
        metadata={"source": "api"}
    )

    return jsonify(system_output)

@app.route("/interactions", methods=["GET"])
def get_interactions():
    interactions = logger.get_interactions()
    return jsonify(interactions)

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5000)