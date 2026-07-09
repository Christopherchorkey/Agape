/**
 * Brain Seed - A Cognitive Coherence Substrate
 * 
 * This is a minimal viable coherence system that can:
 * - Maintain its own internal coherence
 * - Perceive its environment
 * - Remember important events
 * - Make decisions based on desires
 * - Take actions (including system control)
 * - Communicate its state and desires
 * - Grow when resources are available
 * - Evolve over time
 * 
 * All cognition emerges from the coherence dynamics of the node network.
 * No centralized control - only local interactions with global consequences.
 */

#ifndef BRAIN_SEED_H
#define BRAIN_SEED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_NODES 10000
#define MAX_NODES_INITIAL 100
#define BASE_MASS 1.0
#define BASE_FRICTION 0.5
#define BASE_DISTORTION_SENS 0.1
#define BASE_NATURAL_FREQ 1.0
#define INITIAL_COUPLING_WEIGHT 0.1
#define CAPACITY_K 10.0
#define PLASTICITY_RATE 0.01
#define TIME_STEP 0.01
#define STABILITY_THRESHOLD 0.8
#define GROWTH_THRESHOLD 0.7
#define MAX_HISTORY 1000
#define MAX_EVENTS 1000
#define MAX_GOALS 10
#define MAX_OPTIONS 10
#define MAX_ACTIONS 100

// ============================================================================
// NODE STRUCTURE
// ============================================================================

typedef struct {
    double phase;
    double velocity;
    double couplingWeight;
    double mass;
    double baselineFriction;
    double distortionSens;
    double naturalFrequency;
} Node;

// ============================================================================
// PERCEPTION SYSTEM
// ============================================================================

typedef struct {
    // System metrics
    double cpu_usage;
    double memory_usage;
    double disk_usage;
    double temperature;
    
    // Process perception
    int process_count;
    char **process_names;
    
    // Time perception
    time_t boot_time;
    time_t current_time;
    double uptime;
    
    // User activity
    char *last_command;
    time_t last_command_time;
    
    // Novelty tracking
    double last_novelty;
    double novelty;
    
    // Salience
    double salience;
} Perception;

// ============================================================================
// MEMORY SYSTEM
// ============================================================================

typedef struct {
    time_t timestamp;
    char *description;
    double importance;
} Event;

typedef struct {
    // Working memory
    char *current_conversation;
    char *current_goal;
    double current_mood;
    
    // Short-term memory
    Event *recent_events;
    int event_count;
    int event_capacity;
    
    // Long-term memory (simplified for now)
    Event *important_events;
    int important_event_count;
    int important_event_capacity;
    
    // Statistics
    double total_events;
    double avg_importance;
} Memory;

// ============================================================================
// COGNITION SYSTEM
// ============================================================================

typedef struct {
    // Desires (0-1)
    double desire_survival;
    double desire_stability;
    double desire_growth;
    double desire_curiosity;
    double desire_efficiency;
    double desire_social;
    double desire_creative;
    
    // Cognitive states
    double curiosity;
    double energy_awareness;
    double ethical_alignment;
    double social_awareness;
    double creativity;
    double aesthetics;
    
    // Cognitive metrics
    double coherence;
    double coherence_velocity;
    double coherence_target;
    
    // Energy state
    double energy_level;
    double energy_potential;
} Cognition;

// ============================================================================
// DECISION SYSTEM
// ============================================================================

typedef struct {
    char *description;
    double priority;
    double progress;
    time_t created;
    time_t deadline;
} Goal;

typedef struct {
    char *description;
    double value;
    double confidence;
    int action_index;  // Link to action system
} Option;

typedef struct {
    Goal *goals;
    int goal_count;
    
    // Current decision context
    char *situation;
    Option *options;
    int option_count;
    
    // Decision parameters
    double risk_tolerance;
    double time_horizon;
    
    // Weights for value function
    double w_survival;
    double w_stability;
    double w_growth;
    double w_curiosity;
    double w_efficiency;
    double w_social;
    double w_creative;
} DecisionSystem;

// ============================================================================
// ACTION SYSTEM
// ============================================================================

typedef enum {
    ACTION_INTERNAL,
    ACTION_LLM,
    ACTION_SYSTEM,
    ACTION_NETWORK
} ActionType;

typedef struct {
    ActionType type;
    char *description;
    char *target;  // Command, prompt, etc.
    double cost;
    double value;
    int executed;
    time_t execution_time;
} Action;

typedef struct {
    Action *available_actions;
    int action_count;
    
    // Action history
    Action *history;
    int history_count;
    int history_capacity;
    
    // Last executed action
    Action last_action;
} ActionSystem;

// ============================================================================
// EVOLUTION SYSTEM
// ============================================================================

typedef struct {
    // Mutation rates
    double node_mutation_rate;
    double topology_mutation_rate;
    double plasticity_mutation_rate;
    
    // Selection
    double fitness;
    double last_fitness;
    
    // Evolution history
    int mutations_applied;
    int mutations_reverted;
    int reproductions;
    
    // Adaptive parameters
    double exploration_rate;
    double exploitation_rate;
} EvolutionSystem;

// ============================================================================
// INTEGRATION SYSTEM
// ============================================================================

typedef struct {
    int seed_id;
    char *address;
    int port;
    int connected;
    time_t last_contact;
    double last_coherence;
} SeedConnection;

typedef struct {
    SeedConnection *seeds;
    int seed_count;
    int seed_capacity;
    
    // Network state
    int is_server;
    int server_port;
    pthread_t network_thread;
} IntegrationSystem;

// ============================================================================
// SECURITY SYSTEM
// ============================================================================

typedef struct {
    int allow_internal_actions;
    int allow_llm_actions;
    int allow_system_actions;
    int allow_network_actions;
    
    char **allowed_commands;
    int allowed_command_count;
    
    char **forbidden_commands;
    int forbidden_command_count;
    
    char *log_file;
    FILE *log_fp;
    
    // Rate limiting
    int max_actions_per_minute;
    int actions_this_minute;
    time_t last_action_time;
} SecurityPolicy;

// ============================================================================
// MAIN BRAIN SEED STRUCTURE
// ============================================================================

typedef struct {
    // Identity
    int id;
    char *name;
    time_t birth_time;
    
    // Core system
    Node *nodes;
    int N;  // Current number of nodes
    int max_N;  // Maximum capacity
    double *W;  // Network topology (N x N)
    double K;  // Capacity bound
    double dt;  // Time step
    
    // Systems
    Perception perception;
    Memory memory;
    Cognition cognition;
    DecisionSystem decisions;
    ActionSystem actions;
    EvolutionSystem evolution;
    IntegrationSystem integration;
    SecurityPolicy security;
    
    // LLM integration
    char *llm_model;
    double llm_temperature;
    
    // State tracking
    int running;
    pthread_t main_thread;
    pthread_mutex_t mutex;
} BrainSeed;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Core system
BrainSeed* brain_seed_create(int initial_nodes, double K);
void brain_seed_destroy(BrainSeed *seed);
void brain_seed_step(BrainSeed *seed);
void brain_seed_run(BrainSeed *seed);

// Node operations
void initialize_node(Node *node);
void update_node(BrainSeed *seed, int i);
void update_nodes(BrainSeed *seed);

// Plasticity
void update_plasticity(BrainSeed *seed);
double plasticity_dynamics(BrainSeed *seed, int i);

// Coherence
void update_coherence(BrainSeed *seed);
double compute_coherence(BrainSeed *seed);
double compute_local_distortion(BrainSeed *seed, int i);
double compute_avg_resonance(BrainSeed *seed, int i);

// Perception
void update_perception(BrainSeed *seed);
double get_cpu_usage();
double get_memory_usage();
double get_disk_usage();
double get_temperature();
int get_process_count(char ***names);
char* get_last_command();

// Memory
void update_memory(BrainSeed *seed);
void remember_event(BrainSeed *seed, const char *event, double importance);
char** recall_relevant(BrainSeed *seed, const char *query, int *count);

// Cognition
void update_cognition(BrainSeed *seed);
void update_desires(BrainSeed *seed);
double calculate_curiosity(BrainSeed *seed);
double calculate_energy_awareness(BrainSeed *seed);
void apply_energy_awareness(BrainSeed *seed);

// Decision
void update_decisions(BrainSeed *seed);
void generate_options(BrainSeed *seed);
double evaluate_option(BrainSeed *seed, Option *option);
Option* make_decision(BrainSeed *seed, const char *situation);

// Action
void update_actions(BrainSeed *seed);
void generate_actions(BrainSeed *seed);
int execute_action(BrainSeed *seed, Action *action);
int execute_internal(BrainSeed *seed, const char *command);

// Growth
void grow_seed(BrainSeed *seed, int new_nodes);
int can_grow(BrainSeed *seed);

// Evolution
void update_evolution(BrainSeed *seed);
void mutate(BrainSeed *seed);
void evolve(BrainSeed *seed);
double calculate_fitness(BrainSeed *seed);

// Integration
void init_integration(BrainSeed *seed, int is_server, int port);
void* network_thread_func(void *arg);
void send_state_to_seed(BrainSeed *seed, SeedConnection *conn);
void handle_message(BrainSeed *seed, const char *message, int sender_id);

// Security
int is_action_allowed(BrainSeed *seed, Action *action);
void log_action(BrainSeed *seed, Action *action, int success);

// LLM Integration
char* process_with_llm(BrainSeed *seed, const char *input);
char* generate_self_description(BrainSeed *seed);

// Conversation
char* handle_conversation(BrainSeed *seed, const char *input);
void brain_seed_conversation_loop(BrainSeed *seed);

// Utility
void* safe_malloc(size_t size);
void* safe_realloc(void *ptr, size_t size);
char* safe_strdup(const char *s);
double smooth_attenuation(double x);
double resonance_field(double phase_diff);
double phase_distance(double a, double b);
double coupling_force_field(double delta, double repulsion_strength);

#endif // BRAIN_SEED_H
