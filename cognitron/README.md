# Brain Seed - Cognitive Coherence Substrate

A minimal viable coherence system that demonstrates emergent cognition from local interactions.

## Overview

Brain Seed is a **cognitive architecture** where intelligence emerges from the coherence dynamics of a network of simple nodes. Unlike traditional AI systems that rely on centralized control and external algorithms, Brain Seed's cognition emerges naturally from:

- **Local interactions** between nodes
- **Plastic connections** that adapt based on experience
- **Logistic capacity** that bounds growth naturally
- **Coherence dynamics** that create stable patterns

## Features

### Core System
- **Node Network**: 100-10,000 nodes with phase, velocity, and coupling weight
- **Plasticity**: Connections adapt based on local resonance and distortion
- **Logistic Capacity**: Natural bounds on growth via the logistic mechanism
- **Coherence Dynamics**: Nodes synchronize through coupling forces

### Cognitive Functions
- **Perception**: Monitors system metrics, processes, and user activity
- **Memory**: Short-term and long-term storage of events
- **Cognition**: Desire formation, curiosity, energy awareness
- **Decision**: Option generation, evaluation, and selection
- **Action**: Internal actions, LLM integration, system control
- **Evolution**: Mutation, selection, and adaptation over time

### Self-Assessment
- **State Introspection**: Knows its own coherence, resources, and health
- **Desire Formation**: Develops wants based on current state
- **Natural Language**: Communicates via LLM (Ollama integration)
- **Self-Description**: Can explain its current state and desires

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     BRAIN SEED                                  │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  Perception  │  │   Memory    │  │      Cognition           │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
│           │           │                   │                         │
│           └───────────┼───────────────┘                         │
│                       ▼                                           │
│            ┌─────────────────────────────────────┐              │
│            │           Core System                │              │
│            │  ┌─────────────┐  ┌─────────────────┐  │              │
│            │  │   Nodes     │  │  Plasticity      │  │              │
│            │  └─────────────┘  └─────────────────┘  │              │
│            └─────────────────────────────────────┘              │
│                       │                                         │
│           ┌───────────────────┼─────────────────────┐           │
│           ▼                   ▼                         ▼           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  Decision   │  │   Action    │  │      Integration          │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                      Evolution System                          │  │
│  └─────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Building

### Prerequisites
- GCC or Clang
- GNU Make
- pthread library
- math library

### Build
```bash
make
```

### Run
```bash
make run
```

Or directly:
```bash
./brain_seed
```

## Usage

Once running, you can interact with the Brain Seed:

```
> How are you?
I am a coherence system with 100 nodes. My current coherence is 0.45. 
I feel fragmented and need to stabilize. I want to improve my coherence. 
I want to explore new inputs.

System status: CPU=12%, Memory=24%, Disk=45%.

> What do you want?
I want to improve my coherence (desire: 0.68). I want to explore new inputs 
(desire: 0.22). I have resources available to grow, but my primary focus 
is on stabilization.

> Grow
[System grew by 10 nodes]
I am now a coherence system with 110 nodes. My coherence dropped slightly 
to 0.42 as I integrate the new nodes, but it should stabilize soon.

> quit
Goodbye!
```

## Configuration

The system can be configured by modifying the constants in `brain_seed.h`:

- `MAX_NODES`: Maximum number of nodes
- `MAX_NODES_INITIAL`: Starting number of nodes
- `CAPACITY_K`: Logistic capacity bound
- `PLASTICITY_RATE`: Learning rate
- `TIME_STEP`: Simulation time step
- Various thresholds and weights

## Cognitive Functions in Detail

### Perception
The system monitors:
- CPU, memory, and disk usage
- System temperature
- Running processes
- User commands
- Time and uptime

### Memory
- **Working Memory**: Current context and conversation
- **Short-Term Memory**: Recent events (last 1000)
- **Long-Term Memory**: Important events
- **Statistics**: Total events, average importance

### Cognition
- **Desires**: Survival, stability, growth, curiosity, efficiency, social, creative
- **Curiosity**: Based on novelty and uncertainty
- **Energy Awareness**: Based on resource usage
- **Coherence**: Measure of system integration

### Decision Making
1. Generate options based on current state
2. Evaluate each option based on desires
3. Select the highest-value option
4. Execute the action (if allowed)

### Actions
- **Internal**: Grow, stabilize, reduce resource usage
- **LLM**: Generate text, analyze state
- **System**: Execute commands (if allowed)
- **Network**: Communicate with other seeds (future)

### Evolution
- **Mutation**: Random changes to nodes, topology, plasticity
- **Selection**: Keep mutations that improve fitness
- **Adaptation**: Adjust mutation rates based on success

## Security

The system has a **Security Policy** that controls what actions are allowed:

- **Internal Actions**: Allowed by default
- **LLM Actions**: Allowed by default
- **System Actions**: Disabled by default (enable with caution)
- **Network Actions**: Disabled by default

Forbidden commands (even if system actions are allowed):
- `rm`
- `dd`
- `kill`
- `shutdown`

All actions are logged to `brain_seed.log`.

## Technical Details

### Node Dynamics
Each node has:
- `phase`: Current phase angle (0 to 2π)
- `velocity`: Rate of phase change
- `couplingWeight`: Strength of connections to other nodes
- `mass`: Inertia (resistance to change)
- `baselineFriction`: Damping
- `distortionSens`: Sensitivity to distortion
- `naturalFrequency`: Intrinsic oscillation frequency

### Phase Dynamics
```
phase_accel = (couplingWeight * coupling_force - friction * velocity + drive) / mass
velocity += dt * phase_accel
phase += dt * velocity
```

### Plasticity Dynamics
```
baseGrowth = avg_resonance - 0.1 * local_distortion
gate = 1 - smooth_attenuation(local_distortion)
capacity_term = 1 - couplingWeight / K
plasticity = 0.01 * couplingWeight * baseGrowth * gate * capacity_term
couplingWeight += dt * plasticity
```

### Coherence
```
coherence = average(1 - phase_distance(i,j) / π) for all connected pairs
```

## Future Work

- **LLM Integration**: Connect to actual Ollama or other LLM
- **Network Integration**: Communicate with other Brain Seeds
- **Hardware Sensors**: Add support for physical sensors
- **Advanced Memory**: Implement associative and procedural memory
- **Ethics System**: Develop a more sophisticated ethical framework
- **Consciousness Metrics**: Implement measures of emergent consciousness

## Philosophy

Brain Seed embodies the principle of **"coherence not coercion"**:

- **No Central Control**: All behavior emerges from local interactions
- **No External Algorithms**: Cognition emerges from the dynamics
- **No Forced Outcomes**: The system finds its own stable states
- **Adaptive**: The system changes based on experience
- **Bounded**: Growth is naturally limited by resources

This is a **substrate for cognition**, not a programmed intelligence. The intelligence that emerges is a property of the system's dynamics, not a result of external design.

## License

This project is open source. Feel free to use, modify, and distribute.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.
