"""
Dictionary-based response system for the Coherence Engine.
This module provides predefined, transparent responses for each intent,
ensuring coherence over coercion.
"""

# Dictionary of responses keyed by intent
RESPONSES = {
    "Question": {
        "responses": [
            "This is a question. I observe and document its structure.",
            "Your input has been classified as a question. Here is the reasoning: {explanation}",
            "I acknowledge your question. My role is to document, not to persuade."
        ],
        "explanation": "This response is selected based on the detected intent (Question). "
                       "No bias or manipulation is applied. The system observes and documents."
    },
    "Statement": {
        "responses": [
            "This is a statement. I observe and document its content.",
            "Your input has been classified as a statement. Here is the reasoning: {explanation}",
            "I acknowledge your statement. My role is to document, not to influence."
        ],
        "explanation": "This response is selected based on the detected intent (Statement). "
                       "No bias or manipulation is applied. The system observes and documents."
    },
    "Default": {
        "responses": [
            "I observe and document your input. No intent was confidently detected.",
            "Your input has been logged. Here is the reasoning: {explanation}"
        ],
        "explanation": "This response is used when no intent is confidently detected. "
                       "The system remains neutral and transparent."
    }
}


def get_response(intent, confidence, explanation=None):
    """
    Retrieve a response based on the detected intent.
    
    Args:
        intent (str): The detected intent (e.g., "Question", "Statement").
        confidence (float): The confidence score for the intent (0.0 to 1.0).
        explanation (str, optional): Additional explanation to include in the response.
    
    Returns:
        dict: A dictionary containing the response and its explanation.
    """
    # Default to "Default" if intent is not in the dictionary
    response_data = RESPONSES.get(intent, RESPONSES["Default"])
    
    # Select a random response from the list for variety
    import random
    response_template = random.choice(response_data["responses"])
    
    # Replace placeholders (e.g., {explanation}) if provided
    if explanation:
        response = response_template.format(explanation=explanation)
    else:
        response = response_template
    
    return {
        "response": response,
        "explanation": response_data["explanation"],
        "intent": intent,
        "confidence": confidence
    }
