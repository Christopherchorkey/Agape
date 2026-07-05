# Coherence Engine: A Public Example of Coherence Over Coercion

This project demonstrates how to build an AI system that **observes, documents, and explains** its reasoning without manipulation. It combines:
- **Bayesian Classifier**: For intent detection (e.g., "Question" vs. "Statement").
- **Markov Chain**: For generating coherent responses based on observed patterns.
- **Transparency Tools**: Logging all interactions and providing mathematical explanations.

## Key Principles
1. **Coherence Over Coercion**: The system does not sway or bias outcomes. It observes patterns and documents its reasoning.
2. **Mathematical Foundation**: All decisions are grounded in probability and statistics.
3. **Public Auditability**: Every interaction is logged with full explanations.

## Usage

### CLI
Run the CLI for direct interaction:
```bash
python cli.py
```
Example:
```
> What is the meaning of life
--- Coherence Engine Response ---
Intent: Question (Confidence: 99.99%)
Entropy (Uncertainty): 0.01
Response: what is the meaning of life
---------------------------------
```

### API
Start the Flask server:
```bash
python app.py
```
Send a POST request to `http://localhost:5000/interact`:
```json
{"input": "What is the meaning of life"}
```
Response:
```json
{
  "intent": "Question",
  "response": "what is the meaning of life",
  "confidence": 0.9999,
  "entropy": 0.01,
  "explanation": {
    "bayesian": { ... },
    "markov": { ... }
  }
}
```

### View Interactions
All interactions are logged in `interactions.json`. Retrieve them via:
```bash
curl http://localhost:5000/interactions
```

## Training Data
Edit `train_data.json` to customize the Bayesian and Markov models.

## License
Public domain. Use as a reference for coherent, non-coercive AI design.