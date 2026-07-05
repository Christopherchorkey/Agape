"""
Dictionary-based response system for the Coherence Engine.
This module provides predefined, transparent responses for each intent,
ensuring coherence over coercion while resonating with the user's input.
"""

# Dictionary of responses keyed by intent
RESPONSES = {
    "Question": {
        "responses": [
            "You asked: '{input}'. I observe and document this question.",
            "'{input}' is a question. Here is my transparent response: I acknowledge and document it.",
            "I hear your question: '{input}'. My role is to observe and record, not to persuade."
        ],
        "explanation": "This response reflects your input and acknowledges it as a question. "
                       "No judgment or manipulation is applied. The system observes and documents."
    },
    "Statement": {
        "responses": [
            "You stated: '{input}'. I observe and document this statement.",
            "'{input}' is a statement. I acknowledge and record it transparently.",
            "I hear your statement: '{input}'. My role is to document, not to influence."
        ],
        "explanation": "This response reflects your input and acknowledges it as a statement. "
                       "No judgment or manipulation is applied. The system observes and documents."
    },
    "Default": {
        "responses": [
            "You said: '{input}'. I observe and document your input.",
            "'{input}' has been logged. Here is my transparent response: I acknowledge it."
        ],
        "explanation": "This response reflects your input when no intent is confidently detected. "
                       "The system remains neutral and transparent."
    }
}


def get_response(intent, confidence, explanation=None, user_input=None):
    """
    Retrieve a response based on the detected intent.
    
    Args:
        intent (str): The detected intent (e.g., "Question", "Statement").
        confidence (float): The confidence score for the intent (0.0 to 1.0).
        explanation (str, optional): Additional explanation to include in the response.
        user_input (str, optional): The user's input to reflect in the response.
    
    Returns:
        dict: A dictionary containing the response and its explanation.
    """
    # Default to "Default" if intent is not in the dictionary
    response_data = RESPONSES.get(intent, RESPONSES["Default"])
    
    # Select a random response from the list for variety
    import random
    response_template = random.choice(response_data["responses"])
    
    # Replace placeholders (e.g., {input}) if provided
    if user_input:
        response = response_template.format(input=user_input)
    else:
        response = response_template
    
    return {
        "response": response,
        "explanation": response_data["explanation"],
        "intent": intent,
        "confidence": confidence
    }