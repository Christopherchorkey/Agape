/**
 * Brain Seed - Core Implementation
 * 
 * This file contains the core implementation of the brain seed cognitive substrate.
 */

#include "brain_seed.h"
#include <stdarg.h>
#include <ctype.h>
#include <sys/statvfs.h>

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Fatal: Out of memory\n");
        exit(1);
    }
    return ptr;
}

void* safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "Fatal: Out of memory\n");
        exit(1);
    }
    return new_ptr;
}

char* safe_strdup(const char *s) {
    if (!s) return NULL;
    char *copy = strdup(s);
    if (!copy) {
        fprintf(stderr, "Fatal: Out of memory\n");
        exit(1);
    }
    return copy;
}

// ============================================================================
// MATHEMATICAL FUNCTIONS
// ============================================================================

double smooth_attenuation(double x) {
    return 1.0 / (1.0 + exp(x));
}

double resonance_field(double phase_diff) {
    double c = cos(phase_diff);
    return (1.0 + c) / 2.0 * (1.0 / (1.0 + exp(-8.0 * c)));
}

double phase_distance(double a, double b) {
    double diff = fabs(a - b);
    // Normalize to [0, 2π]
    while (diff > 2 * M_PI) diff -= 2 * M_PI;
    while (diff < 0) diff += 2 * M_PI;
    // Take the shorter arc
    if (diff > M_PI) diff = 2 * M_PI - diff;
    return 2.0 * asin(fabs(sin(diff / 2.0)));
}

double coupling_force_field(double delta, double repulsion_strength) {
    double rf = resonance_field(delta);
    double rg = smooth_attenuation(20.0 * pow(phase_distance(delta, 0.0), 2));
    return (rf - repulsion_strength * rg) * sin(delta);
}

// ============================================================================
// SYSTEM METRICS
// ============================================================================

double get_cpu_usage() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return 0.0;
    }
    
    // Simple approximation: load average
    // For more accurate CPU usage, we'd need to track process times
    // This is a placeholder
    return info.loads[0] / (double)(1 << SI_LOAD_SHIFT);
}

double get_memory_usage() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return 0.0;
    }
    
    // Total used memory / total memory
    double total = (double)info.totalram * info.mem_unit;
    double free = (double)info.freeram * info.mem_unit;
    double used = total - free;
    
    return used / total;
}

double get_disk_usage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        return 0.0;
    }
    
    double total = (double)stat.f_blocks * stat.f_frsize;
    double free = (double)stat.f_bfree * stat.f_frsize;
    double used = total - free;
    
    return used / total;
}

double get_temperature() {
    // Try to read CPU temperature from /sys/class/thermal
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) {
        return 0.0;
    }
    
    int temp;
    if (fscanf(fp, "%d", &temp) != 1) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);
    
    // Convert from millidegrees to degrees
    return (double)temp / 1000.0;
}

int get_process_count(char ***names) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        *names = NULL;
        return 0;
    }
    
    struct dirent *entry;
    int count = 0;
    char **tmp_names = NULL;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit((unsigned char)entry->d_name[0])) {
            count++;
            tmp_names = safe_realloc(tmp_names, count * sizeof(char*));
            tmp_names[count - 1] = safe_strdup(entry->d_name);
        }
    }
    closedir(dir);
    
    *names = tmp_names;
    return count;
}

char* get_last_command() {
    // Try to read from bash history
    char *home = getenv("HOME");
    if (!home) return NULL;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/.bash_history", home);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return NULL;
    }
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *last = NULL;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        if (line[0] != '#' && line[0] != '\n') {
            free(last);
            last = safe_strdup(line);
        }
    }
    
    free(line);
    fclose(fp);
    
    return last;
}

// ============================================================================
// NODE OPERATIONS
// ============================================================================

void initialize_node(Node *node) {
    node->phase = 2.0 * M_PI * ((double)rand() / RAND_MAX);
    node->velocity = 0.0;
    node->couplingWeight = INITIAL_COUPLING_WEIGHT;
    node->mass = BASE_MASS;
    node->baselineFriction = BASE_FRICTION;
    node->distortionSens = BASE_DISTORTION_SENS;
    node->naturalFrequency = BASE_NATURAL_FREQ;
}

// ============================================================================
// CORE SYSTEM
// ============================================================================

BrainSeed* brain_seed_create(int initial_nodes, double K) {
    BrainSeed *seed = safe_malloc(sizeof(BrainSeed));
    
    // Initialize identity
    seed->id = rand() % 10000;
    seed->name = safe_strdup("BrainSeed");
    seed->birth_time = time(NULL);
    
    // Initialize core system
    seed->N = initial_nodes;
    seed->max_N = MAX_NODES;
    seed->K = K;
    seed->dt = TIME_STEP;
    
    seed->nodes = safe_malloc(seed->max_N * sizeof(Node));
    seed->W = safe_malloc(seed->max_N * seed->max_N * sizeof(double));
    
    // Initialize nodes
    for (int i = 0; i < seed->N; i++) {
        initialize_node(&seed->nodes[i]);
    }
    
    // Initialize topology (sparse random)
    for (int i = 0; i < seed->max_N; i++) {
        for (int j = 0; j < seed->max_N; j++) {
            if (i < seed->N && j < seed->N) {
                // 10% connection probability
                seed->W[i * seed->max_N + j] = ((double)rand() / RAND_MAX) < 0.1 ? 1.0 : 0.0;
            } else {
                seed->W[i * seed->max_N + j] = 0.0;
            }
        }
    }
    
    // Initialize perception
    seed->perception.cpu_usage = 0.0;
    seed->perception.memory_usage = 0.0;
    seed->perception.disk_usage = 0.0;
    seed->perception.temperature = 0.0;
    seed->perception.process_count = 0;
    seed->perception.process_names = NULL;
    seed->perception.boot_time = time(NULL);
    seed->perception.current_time = seed->perception.boot_time;
    seed->perception.uptime = 0.0;
    seed->perception.last_command = NULL;
    seed->perception.last_command_time = 0;
    seed->perception.last_novelty = 0.0;
    seed->perception.novelty = 0.0;
    seed->perception.salience = 0.0;
    
    // Initialize memory
    seed->memory.current_conversation = NULL;
    seed->memory.current_goal = NULL;
    seed->memory.current_mood = 0.5;  // Neutral
    seed->memory.event_capacity = MAX_EVENTS;
    seed->memory.event_count = 0;
    seed->memory.recent_events = safe_malloc(MAX_EVENTS * sizeof(Event));
    for (int i = 0; i < MAX_EVENTS; i++) {
        seed->memory.recent_events[i].description = NULL;
    }
    seed->memory.important_event_capacity = MAX_EVENTS;
    seed->memory.important_event_count = 0;
    seed->memory.important_events = safe_malloc(MAX_EVENTS * sizeof(Event));
    for (int i = 0; i < MAX_EVENTS; i++) {
        seed->memory.important_events[i].description = NULL;
    }
    seed->memory.total_events = 0.0;
    seed->memory.avg_importance = 0.0;
    
    // Initialize cognition
    seed->cognition.desire_survival = 0.0;
    seed->cognition.desire_stability = 0.0;
    seed->cognition.desire_growth = 0.0;
    seed->cognition.desire_curiosity = 0.0;
    seed->cognition.desire_efficiency = 0.0;
    seed->cognition.desire_social = 0.0;
    seed->cognition.desire_creative = 0.0;
    seed->cognition.curiosity = 0.0;
    seed->cognition.energy_awareness = 0.0;
    seed->cognition.ethical_alignment = 0.0;
    seed->cognition.social_awareness = 0.0;
    seed->cognition.creativity = 0.0;
    seed->cognition.aesthetics = 0.0;
    seed->cognition.coherence = 0.0;
    seed->cognition.coherence_velocity = 0.0;
    seed->cognition.coherence_target = STABILITY_THRESHOLD;
    seed->cognition.energy_level = 1.0;
    seed->cognition.energy_potential = 1.0;
    
    // Initialize decisions
    seed->decisions.goals = safe_malloc(MAX_GOALS * sizeof(Goal));
    seed->decisions.goal_count = 0;
    for (int i = 0; i < MAX_GOALS; i++) {
        seed->decisions.goals[i].description = NULL;
    }
    seed->decisions.situation = NULL;
    seed->decisions.options = safe_malloc(MAX_OPTIONS * sizeof(Option));
    seed->decisions.option_count = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        seed->decisions.options[i].description = NULL;
    }
    seed->decisions.risk_tolerance = 0.5;
    seed->decisions.time_horizon = 10.0;
    seed->decisions.w_survival = 1.0;
    seed->decisions.w_stability = 0.8;
    seed->decisions.w_growth = 0.6;
    seed->decisions.w_curiosity = 0.5;
    seed->decisions.w_efficiency = 0.4;
    seed->decisions.w_social = 0.3;
    seed->decisions.w_creative = 0.2;
    
    // Initialize actions
    seed->actions.available_actions = safe_malloc(MAX_ACTIONS * sizeof(Action));
    seed->actions.action_count = 0;
    for (int i = 0; i < MAX_ACTIONS; i++) {
        seed->actions.available_actions[i].description = NULL;
        seed->actions.available_actions[i].target = NULL;
    }
    seed->actions.history_capacity = MAX_ACTIONS;
    seed->actions.history_count = 0;
    seed->actions.history = safe_malloc(MAX_ACTIONS * sizeof(Action));
    for (int i = 0; i < MAX_ACTIONS; i++) {
        seed->actions.history[i].description = NULL;
        seed->actions.history[i].target = NULL;
    }
    seed->actions.last_action.description = NULL;
    seed->actions.last_action.target = NULL;
    
    // Initialize evolution
    seed->evolution.node_mutation_rate = 0.001;
    seed->evolution.topology_mutation_rate = 0.0001;
    seed->evolution.plasticity_mutation_rate = 0.00001;
    seed->evolution.fitness = 0.0;
    seed->evolution.last_fitness = 0.0;
    seed->evolution.mutations_applied = 0;
    seed->evolution.mutations_reverted = 0;
    seed->evolution.reproductions = 0;
    seed->evolution.exploration_rate = 0.7;
    seed->evolution.exploitation_rate = 0.3;
    
    // Initialize integration
    seed->integration.seeds = safe_malloc(10 * sizeof(SeedConnection));
    seed->integration.seed_count = 0;
    seed->integration.seed_capacity = 10;
    seed->integration.is_server = 0;
    seed->integration.server_port = 0;
    
    // Initialize security
    seed->security.allow_internal_actions = 1;
    seed->security.allow_llm_actions = 1;
    seed->security.allow_system_actions = 0;  // Disabled by default
    seed->security.allow_network_actions = 0;  // Disabled by default
    seed->security.allowed_commands = NULL;
    seed->security.allowed_command_count = 0;
    seed->security.forbidden_commands = safe_malloc(10 * sizeof(char*));
    seed->security.forbidden_commands[0] = safe_strdup("rm");
    seed->security.forbidden_commands[1] = safe_strdup("dd");
    seed->security.forbidden_commands[2] = safe_strdup("kill");
    seed->security.forbidden_commands[3] = safe_strdup("shutdown");
    seed->security.forbidden_command_count = 4;
    seed->security.log_file = safe_strdup("brain_seed.log");
    seed->security.log_fp = fopen(seed->security.log_file, "a");
    seed->security.max_actions_per_minute = 60;
    seed->security.actions_this_minute = 0;
    seed->security.last_action_time = time(NULL);
    
    // LLM settings
    seed->llm_model = safe_strdup("llama2");
    seed->llm_temperature = 0.7;
    
    // State
    seed->running = 0;
    pthread_mutex_init(&seed->mutex, NULL);
    
    // Initial coherence calculation
    update_coherence(seed);
    
    return seed;
}

void brain_seed_destroy(BrainSeed *seed) {
    if (!seed) return;
    
    // Stop running
    seed->running = 0;
    
    // Free nodes
    free(seed->nodes);
    free(seed->W);
    
    // Free perception
    if (seed->perception.process_names) {
        for (int i = 0; i < seed->perception.process_count; i++) {
            free(seed->perception.process_names[i]);
        }
        free(seed->perception.process_names);
    }
    free(seed->perception.last_command);
    
    // Free memory
    for (int i = 0; i < seed->memory.event_count; i++) {
        free(seed->memory.recent_events[i].description);
    }
    free(seed->memory.recent_events);
    
    for (int i = 0; i < seed->memory.important_event_count; i++) {
        free(seed->memory.important_events[i].description);
    }
    free(seed->memory.important_events);
    free(seed->memory.current_conversation);
    free(seed->memory.current_goal);
    
    // Free decisions
    for (int i = 0; i < seed->decisions.goal_count; i++) {
        free(seed->decisions.goals[i].description);
    }
    free(seed->decisions.goals);
    free(seed->decisions.situation);
    for (int i = 0; i < seed->decisions.option_count; i++) {
        free(seed->decisions.options[i].description);
    }
    free(seed->decisions.options);
    
    // Free actions
    for (int i = 0; i < seed->actions.action_count; i++) {
        free(seed->actions.available_actions[i].description);
        free(seed->actions.available_actions[i].target);
    }
    free(seed->actions.available_actions);
    
    for (int i = 0; i < seed->actions.history_count; i++) {
        free(seed->actions.history[i].description);
        free(seed->actions.history[i].target);
    }
    free(seed->actions.history);
    free(seed->actions.last_action.description);
    free(seed->actions.last_action.target);
    
    // Free integration
    for (int i = 0; i < seed->integration.seed_count; i++) {
        free(seed->integration.seeds[i].address);
    }
    free(seed->integration.seeds);
    
    // Free security
    for (int i = 0; i < seed->security.forbidden_command_count; i++) {
        free(seed->security.forbidden_commands[i]);
    }
    free(seed->security.forbidden_commands);
    if (seed->security.log_fp) fclose(seed->security.log_fp);
    free(seed->security.log_file);
    
    // Free LLM
    free(seed->llm_model);
    
    // Free identity
    free(seed->name);
    
    pthread_mutex_destroy(&seed->mutex);
    free(seed);
}

// ============================================================================
// COHERENCE CALCULATIONS
// ============================================================================

double compute_local_distortion(BrainSeed *seed, int i) {
    double distortion = 0.0;
    int count = 0;
    
    for (int j = 0; j < seed->N; j++) {
        if (seed->W[i * seed->max_N + j] > 0.01) {
            double delta = phase_distance(seed->nodes[i].phase, seed->nodes[j].phase);
            distortion += seed->W[i * seed->max_N + j] * delta * delta;
            count++;
        }
    }
    
    if (count > 0) {
        distortion /= count;
    }
    
    return distortion;
}

double compute_avg_resonance(BrainSeed *seed, int i) {
    double resonance = 0.0;
    int count = 0;
    
    for (int j = 0; j < seed->N; j++) {
        if (seed->W[i * seed->max_N + j] > 0.01) {
            double delta = seed->nodes[j].phase - seed->nodes[i].phase;
            resonance += resonance_field(delta);
            count++;
        }
    }
    
    if (count > 0) {
        resonance /= count;
    }
    
    return resonance;
}

double compute_coherence(BrainSeed *seed) {
    double total_coherence = 0.0;
    int count = 0;
    
    for (int i = 0; i < seed->N; i++) {
        for (int j = i + 1; j < seed->N; j++) {
            if (seed->W[i * seed->max_N + j] > 0.01) {
                double delta = phase_distance(seed->nodes[i].phase, seed->nodes[j].phase);
                // Coherence is inverse of distance
                total_coherence += seed->W[i * seed->max_N + j] * (1.0 - delta / M_PI);
                count++;
            }
        }
    }
    
    if (count > 0) {
        total_coherence /= count;
    }
    
    return total_coherence;
}

void update_coherence(BrainSeed *seed) {
    double old_coherence = seed->cognition.coherence;
    seed->cognition.coherence = compute_coherence(seed);
    seed->cognition.coherence_velocity = seed->cognition.coherence - old_coherence;
}

// ============================================================================
// NODE UPDATE
// ============================================================================

void update_node(BrainSeed *seed, int i) {
    Node *node = &seed->nodes[i];
    double coupling_force = 0.0;
    
    // Compute coupling force from all connected nodes
    for (int j = 0; j < seed->N; j++) {
        if (seed->W[i * seed->max_N + j] > 0.01) {
            double delta = seed->nodes[j].phase - node->phase;
            coupling_force += seed->W[i * seed->max_N + j] * coupling_force_field(delta, 3.0);
        }
    }
    
    // Compute local distortion
    double d_i = compute_local_distortion(seed, i);
    
    // Compute friction
    double friction = node->baselineFriction * smooth_attenuation(d_i) + 
                     node->distortionSens * d_i;
    
    // Compute drive
    double drive = node->baselineFriction * 0.5 * node->naturalFrequency;
    
    // Update phase and velocity
    double phase_accel = (node->couplingWeight * coupling_force - 
                         friction * node->velocity + drive) / node->mass;
    
    node->phase += seed->dt * node->velocity;
    node->velocity += seed->dt * phase_accel;
    
    // Normalize phase to [0, 2π]
    while (node->phase >= 2 * M_PI) node->phase -= 2 * M_PI;
    while (node->phase < 0) node->phase += 2 * M_PI;
}

void update_nodes(BrainSeed *seed) {
    for (int i = 0; i < seed->N; i++) {
        update_node(seed, i);
    }
}

// ============================================================================
// PLASTICITY
// ============================================================================

double plasticity_dynamics(BrainSeed *seed, int i) {
    Node *node = &seed->nodes[i];
    double d_i = compute_local_distortion(seed, i);
    double avg_resonance = compute_avg_resonance(seed, i);
    double baseGrowth = avg_resonance - 0.1 * d_i;
    double gate = 1.0 - smooth_attenuation(d_i);
    double capacity_term = 1.0 - node->couplingWeight / seed->K;
    
    // Logistic capacity mechanism
    return PLASTICITY_RATE * node->couplingWeight * baseGrowth * gate * capacity_term;
}

void update_plasticity(BrainSeed *seed) {
    static int counter = 0;
    counter++;
    
    // Only update plasticity every 10 steps
    if (counter % 10 != 0) return;
    
    for (int i = 0; i < seed->N; i++) {
        double plasticity = plasticity_dynamics(seed, i);
        seed->nodes[i].couplingWeight += seed->dt * plasticity;
        
        // Ensure bounds
        if (seed->nodes[i].couplingWeight > seed->K) {
            seed->nodes[i].couplingWeight = seed->K;
        }
        if (seed->nodes[i].couplingWeight < 0) {
            seed->nodes[i].couplingWeight = 0;
        }
    }
}

// ============================================================================
// PERCEPTION
// ============================================================================

void update_perception(BrainSeed *seed) {
    Perception *p = &seed->perception;
    
    // Update time
    p->current_time = time(NULL);
    p->uptime = difftime(p->current_time, p->boot_time);
    
    // Update system metrics
    p->cpu_usage = get_cpu_usage();
    p->memory_usage = get_memory_usage();
    p->disk_usage = get_disk_usage();
    p->temperature = get_temperature();
    
    // Update process info
    if (p->process_names) {
        for (int i = 0; i < p->process_count; i++) {
            free(p->process_names[i]);
        }
        free(p->process_names);
    }
    p->process_count = get_process_count(&p->process_names);
    
    // Update user activity
    char *last_cmd = get_last_command();
    if (last_cmd) {
        if (!p->last_command || strcmp(last_cmd, p->last_command) != 0) {
            free(p->last_command);
            p->last_command = last_cmd;
            p->last_command_time = p->current_time;
            
            // Calculate novelty (simple: if command is different from recent)
            p->last_novelty = p->novelty;
            p->novelty = (p->current_time - p->last_command_time > 60) ? 1.0 : 0.5;
        } else {
            free(last_cmd);
        }
    }
    
    // Calculate salience (importance of current state)
    p->salience = p->cpu_usage * 0.3 + p->memory_usage * 0.3 + p->temperature / 100.0 * 0.4;
}

// ============================================================================
// MEMORY
// ============================================================================

void remember_event(BrainSeed *seed, const char *event, double importance) {
    Memory *m = &seed->memory;
    
    // Add to short-term memory
    if (m->event_count >= m->event_capacity) {
        // Shift out oldest
        free(m->recent_events[0].description);
        for (int i = 0; i < m->event_count - 1; i++) {
            m->recent_events[i] = m->recent_events[i + 1];
        }
        m->event_count--;
    }
    
    Event *e = &m->recent_events[m->event_count];
    e->timestamp = time(NULL);
    e->description = safe_strdup(event);
    e->importance = importance;
    m->event_count++;
    
    // If important, store in long-term memory
    if (importance > 0.7) {
        if (m->important_event_count >= m->important_event_capacity) {
            // Shift out oldest
            free(m->important_events[0].description);
            for (int i = 0; i < m->important_event_count - 1; i++) {
                m->important_events[i] = m->important_events[i + 1];
            }
            m->important_event_count--;
        }
        
        Event *ie = &m->important_events[m->important_event_count];
        ie->timestamp = e->timestamp;
        ie->description = safe_strdup(event);
        ie->importance = importance;
        m->important_event_count++;
    }
    
    // Update statistics
    m->total_events++;
    m->avg_importance = (m->avg_importance * (m->total_events - 1) + importance) / m->total_events;
}

// ============================================================================
// COGNITION
// ============================================================================

double calculate_curiosity(BrainSeed *seed) {
    Cognition *c = &seed->cognition;
    Perception *p = &seed->perception;
    
    double novelty = p->novelty;
    double uncertainty = 1.0 - fabs(c->coherence - c->coherence_target);
    double boredom = 1.0 - c->desire_curiosity;
    
    return novelty * uncertainty * (1.0 - boredom);
}

double calculate_energy_awareness(BrainSeed *seed) {
    Perception *p = &seed->perception;
    
    // Energy level based on resource usage
    double cpu_energy = 1.0 - p->cpu_usage;
    double mem_energy = 1.0 - p->memory_usage;
    double disk_energy = 1.0 - p->disk_usage;
    
    return (cpu_energy + mem_energy + disk_energy) / 3.0;
}

void apply_energy_awareness(BrainSeed *seed) {
    double energy = calculate_energy_awareness(seed);
    seed->cognition.energy_level = energy;
    seed->cognition.energy_potential = energy;
    
    for (int i = 0; i < seed->N; i++) {
        // Energy affects node mass (inertia)
        seed->nodes[i].mass = BASE_MASS * (1.0 + energy * 0.5);
        
        // Energy affects friction
        seed->nodes[i].baselineFriction = BASE_FRICTION * (1.0 - 0.3 * (1.0 - energy));
    }
}

void update_desires(BrainSeed *seed) {
    Cognition *c = &seed->cognition;
    Perception *p = &seed->perception;
    
    // Survival: based on resource usage
    c->desire_survival = (1.0 - p->cpu_usage) * (1.0 - p->memory_usage) * (1.0 - p->disk_usage);
    
    // Stability: based on coherence and its rate of change
    c->desire_stability = (1.0 - fabs(c->coherence_velocity)) * (1.0 - fabs(c->coherence - c->coherence_target));
    
    // Growth: based on capacity usage and available resources
    double capacity_usage = 0.0;
    for (int i = 0; i < seed->N; i++) {
        capacity_usage += seed->nodes[i].couplingWeight / seed->K;
    }
    capacity_usage /= seed->N;
    c->desire_growth = capacity_usage * (1.0 - p->cpu_usage) * (1.0 - p->memory_usage);
    
    // Curiosity
    c->curiosity = calculate_curiosity(seed);
    c->desire_curiosity = c->curiosity;
    
    // Efficiency
    c->desire_efficiency = c->coherence / (p->cpu_usage + p->memory_usage + 0.01);
    
    // Social (placeholder - will be updated when integration is added)
    c->desire_social = 0.0;
    
    // Creative (based on coherence and diversity)
    c->desire_creative = c->coherence * (1.0 - c->coherence);
    
    // Normalize desires
    double sum = c->desire_survival + c->desire_stability + c->desire_growth + 
                c->desire_curiosity + c->desire_efficiency + c->desire_social + c->desire_creative;
    if (sum > 0) {
        c->desire_survival /= sum;
        c->desire_stability /= sum;
        c->desire_growth /= sum;
        c->desire_curiosity /= sum;
        c->desire_efficiency /= sum;
        c->desire_social /= sum;
        c->desire_creative /= sum;
    }
}

void update_cognition(BrainSeed *seed) {
    update_coherence(seed);
    apply_energy_awareness(seed);
    update_desires(seed);
}

// ============================================================================
// DECISION SYSTEM
// ============================================================================

void add_option_to_decision(DecisionSystem *ds, const char *desc, double value) {
    if (ds->option_count >= MAX_OPTIONS) return;
    
    ds->options[ds->option_count].description = safe_strdup(desc);
    ds->options[ds->option_count].value = value;
    ds->options[ds->option_count].confidence = 0.5;
    ds->options[ds->option_count].action_index = -1;
    ds->option_count++;
}

char* generate_options_with_llm(BrainSeed *seed, const char *situation) {
    // Placeholder for LLM-generated options
    // In a real implementation, this would use the LLM to generate creative options
    
    char *options = safe_malloc(256);
    strcpy(options, "Explore new input sources\nOptimize current configuration\nRequest more resources");
    return options;
}

void generate_options(BrainSeed *seed) {
    DecisionSystem *ds = &seed->decisions;
    
    // Clear existing options
    for (int i = 0; i < ds->option_count; i++) {
        free(ds->options[i].description);
    }
    ds->option_count = 0;
    
    // Option 1: Do nothing
    add_option_to_decision(ds, "Do nothing", 0.0);
    
    // Option 2: Grow if possible
    if (can_grow(seed)) {
        add_option_to_decision(ds, "Grow by 10 nodes", 0.0);
    }
    
    // Option 3: Stabilize if coherence is low
    if (seed->cognition.coherence < STABILITY_THRESHOLD) {
        add_option_to_decision(ds, "Focus on stabilization", 0.0);
    }
    
    // Option 4: Explore if curious
    if (seed->cognition.desire_curiosity > 0.5) {
        add_option_to_decision(ds, "Seek new input", 0.0);
    }
    
    // Option 5: Optimize resources if inefficient
    if (seed->perception.cpu_usage > 0.8 || seed->perception.memory_usage > 0.8) {
        add_option_to_decision(ds, "Reduce resource usage", 0.0);
    }
    
    // Option 6: Use LLM to generate more options
    if (seed->security.allow_llm_actions) {
        char *llm_options = generate_options_with_llm(seed, ds->situation);
        if (llm_options) {
            // Parse LLM options (simple: one per line)
            char *line = strtok(llm_options, "\n");
            while (line && ds->option_count < MAX_OPTIONS) {
                if (strlen(line) > 0) {
                    add_option_to_decision(ds, line, 0.0);
                }
                line = strtok(NULL, "\n");
            }
            free(llm_options);
        }
    }
}

double evaluate_option(BrainSeed *seed, Option *option) {
    DecisionSystem *ds = &seed->decisions;
    Cognition *c = &seed->cognition;
    
    double value = 0.0;
    
    // Evaluate based on desires
    if (strstr(option->description, "grow") || strstr(option->description, "Grow")) {
        value += c->desire_growth * ds->w_growth;
    }
    if (strstr(option->description, "stabil") || strstr(option->description, "Stabil")) {
        value += c->desire_stability * ds->w_stability;
    }
    if (strstr(option->description, "explore") || strstr(option->description, "Explore")) {
        value += c->desire_curiosity * ds->w_curiosity;
    }
    if (strstr(option->description, "reduce") || strstr(option->description, "optimize") || 
        strstr(option->description, "Reduce") || strstr(option->description, "Optimize")) {
        value += c->desire_efficiency * ds->w_efficiency;
    }
    
    // Survival is always important
    value += c->desire_survival * ds->w_survival;
    
    return value;
}

Option* make_decision(BrainSeed *seed, const char *situation) {
    DecisionSystem *ds = &seed->decisions;
    
    // Set current situation
    free(ds->situation);
    ds->situation = safe_strdup(situation);
    
    // Generate options
    generate_options(seed);
    
    // Evaluate each option
    for (int i = 0; i < ds->option_count; i++) {
        ds->options[i].value = evaluate_option(seed, &ds->options[i]);
    }
    
    // Select best option
    Option *best = &ds->options[0];
    for (int i = 1; i < ds->option_count; i++) {
        if (ds->options[i].value > best->value) {
            best = &ds->options[i];
        }
    }
    
    // Return a copy
    Option *result = safe_malloc(sizeof(Option));
    *result = *best;
    result->description = safe_strdup(best->description);
    
    return result;
}

// ============================================================================
// ACTION SYSTEM
// ============================================================================

void add_action_to_system(ActionSystem *as, ActionType type, const char *desc, const char *target, double cost) {
    if (as->action_count >= MAX_ACTIONS) return;
    
    Action *a = &as->available_actions[as->action_count];
    a->type = type;
    a->description = safe_strdup(desc);
    a->target = safe_strdup(target);
    a->cost = cost;
    a->value = 0.0;
    a->executed = 0;
    a->execution_time = 0;
    as->action_count++;
}

void generate_actions(BrainSeed *seed) {
    ActionSystem *as = &seed->actions;
    
    // Clear existing actions
    for (int i = 0; i < as->action_count; i++) {
        free(as->available_actions[i].description);
        free(as->available_actions[i].target);
    }
    as->action_count = 0;
    
    // Internal actions
    if (seed->security.allow_internal_actions) {
        add_action_to_system(as, ACTION_INTERNAL, "Do nothing", "", 0.0);
        if (can_grow(seed)) {
            add_action_to_system(as, ACTION_INTERNAL, "Grow by 10 nodes", "grow 10", 0.1);
        }
        add_action_to_system(as, ACTION_INTERNAL, "Focus on stabilization", "stabilize", 0.05);
        add_action_to_system(as, ACTION_INTERNAL, "Reduce resource usage", "reduce", 0.05);
    }
    
    // LLM actions
    if (seed->security.allow_llm_actions) {
        add_action_to_system(as, ACTION_LLM, "Generate self-description", "Describe yourself", 0.5);
        add_action_to_system(as, ACTION_LLM, "Analyze current state", "Analyze my state", 0.5);
    }
    
    // System actions (if allowed)
    if (seed->security.allow_system_actions) {
        add_action_to_system(as, ACTION_SYSTEM, "List processes", "ps aux", 1.0);
        add_action_to_system(as, ACTION_SYSTEM, "Check disk usage", "df -h", 0.5);
    }
}

int execute_internal(BrainSeed *seed, const char *command) {
    if (strcmp(command, "grow 10") == 0) {
        grow_seed(seed, 10);
        return 1;
    } else if (strcmp(command, "stabilize") == 0) {
        seed->cognition.coherence_target = STABILITY_THRESHOLD;
        return 1;
    } else if (strcmp(command, "reduce") == 0) {
        // Reduce node count by 10%
        int reduce_by = seed->N / 10;
        if (reduce_by > 0) {
            // Simple: just reduce N (nodes are still in memory but not used)
            seed->N -= reduce_by;
            if (seed->N < 10) seed->N = 10;
        }
        return 1;
    }
    return 0;
}

int execute_action(BrainSeed *seed, Action *action) {
    if (!is_action_allowed(seed, action)) {
        log_action(seed, action, 0);
        return 0;
    }
    
    int success = 0;
    
    switch (action->type) {
        case ACTION_INTERNAL:
            success = execute_internal(seed, action->target);
            break;
            
        case ACTION_LLM:
            if (seed->security.allow_llm_actions) {
                char *response = process_with_llm(seed, action->target);
                printf("LLM Response: %s\n", response);
                free(response);
                success = 1;
            }
            break;
            
        case ACTION_SYSTEM:
            if (seed->security.allow_system_actions) {
                success = system(action->target) == 0;
            }
            break;
            
        case ACTION_NETWORK:
            // Not implemented yet
            break;
    }
    
    log_action(seed, action, success);
    
    // Update rate limiting
    seed->security.actions_this_minute++;
    seed->security.last_action_time = time(NULL);
    
    return success;
}

int is_action_allowed(BrainSeed *seed, Action *action) {
    SecurityPolicy *sp = &seed->security;
    
    // Check rate limiting
    if (difftime(time(NULL), sp->last_action_time) < 60) {
        if (sp->actions_this_minute >= sp->max_actions_per_minute) {
            return 0;
        }
    } else {
        sp->actions_this_minute = 0;
    }
    
    // Check type permissions
    switch (action->type) {
        case ACTION_INTERNAL:
            return sp->allow_internal_actions;
        case ACTION_LLM:
            return sp->allow_llm_actions;
        case ACTION_SYSTEM:
            return sp->allow_system_actions;
        case ACTION_NETWORK:
            return sp->allow_network_actions;
        default:
            return 0;
    }
    
    // Check forbidden commands
    for (int i = 0; i < sp->forbidden_command_count; i++) {
        if (strstr(action->target, sp->forbidden_commands[i])) {
            return 0;
        }
    }
    
    return 1;
}

void log_action(BrainSeed *seed, Action *action, int success) {
    SecurityPolicy *sp = &seed->security;
    
    if (!sp->log_fp) return;
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
    
    fprintf(sp->log_fp, "[%s] %s: %s (type=%d, target=%s, success=%d)\n",
            timestamp,
            success ? "ACTION" : "DENIED",
            action->description,
            action->type,
            action->target ? action->target : "(null)",
            success);
    fflush(sp->log_fp);
}

// ============================================================================
// GROWTH
// ============================================================================

int can_grow(BrainSeed *seed) {
    // Check if we have room
    if (seed->N >= seed->max_N) {
        return 0;
    }
    
    // Check resource usage
    if (seed->perception.cpu_usage > 0.9) {
        return 0;
    }
    if (seed->perception.memory_usage > 0.9) {
        return 0;
    }
    
    // Check desire
    if (seed->cognition.desire_growth < 0.3) {
        return 0;
    }
    
    return 1;
}

void grow_seed(BrainSeed *seed, int new_nodes) {
    if (new_nodes <= 0) return;
    if (seed->N + new_nodes > seed->max_N) {
        new_nodes = seed->max_N - seed->N;
    }
    
    int old_N = seed->N;
    seed->N += new_nodes;
    
    // Initialize new nodes
    for (int i = old_N; i < seed->N; i++) {
        initialize_node(&seed->nodes[i]);
    }
    
    // Initialize new topology connections
    for (int i = 0; i < seed->N; i++) {
        for (int j = 0; j < seed->N; j++) {
            if (i < old_N && j < old_N) {
                // Keep existing connections
                continue;
            } else if (i >= old_N || j >= old_N) {
                // New connections: 10% probability
                seed->W[i * seed->max_N + j] = ((double)rand() / RAND_MAX) < 0.1 ? 1.0 : 0.0;
            }
        }
    }
    
    // Log the growth
    char event[256];
    snprintf(event, sizeof(event), "Grew from %d to %d nodes", old_N, seed->N);
    remember_event(seed, event, 0.8);
    
    // Update coherence
    update_coherence(seed);
}

// ============================================================================
// EVOLUTION
// ============================================================================

double calculate_fitness(BrainSeed *seed) {
    Cognition *c = &seed->cognition;
    Perception *p = &seed->perception;
    
    double fitness = 0.0;
    
    // Coherence is good
    fitness += c->coherence * 2.0;
    
    // Stability is good
    fitness += (1.0 - fabs(c->coherence_velocity)) * 1.0;
    
    // Resource efficiency is good
    fitness += (1.0 - p->cpu_usage) * 0.5;
    fitness += (1.0 - p->memory_usage) * 0.5;
    
    // Growth potential is good
    fitness += c->desire_growth * 0.5;
    
    return fitness;
}

void mutate_node(Node *node) {
    // Mutate one parameter randomly
    int param = rand() % 5;
    double mutation = ((double)rand() / RAND_MAX) * 0.2 - 0.1;  // -0.1 to +0.1
    
    switch (param) {
        case 0:
            node->naturalFrequency += mutation;
            if (node->naturalFrequency < 0) node->naturalFrequency = 0;
            break;
        case 1:
            node->baselineFriction += mutation;
            if (node->baselineFriction < 0) node->baselineFriction = 0;
            break;
        case 2:
            node->distortionSens += mutation;
            if (node->distortionSens < 0) node->distortionSens = 0;
            break;
        case 3:
            node->mass += mutation;
            if (node->mass < 0.1) node->mass = 0.1;
            break;
        case 4:
            node->couplingWeight += mutation;
            if (node->couplingWeight < 0) node->couplingWeight = 0;
            break;
    }
}

void mutate(BrainSeed *seed) {
    EvolutionSystem *e = &seed->evolution;
    
    // Node mutation
    if ((double)rand() / RAND_MAX < e->node_mutation_rate) {
        int node = rand() % seed->N;
        mutate_node(&seed->nodes[node]);
        e->mutations_applied++;
    }
    
    // Topology mutation
    if ((double)rand() / RAND_MAX < e->topology_mutation_rate) {
        int i = rand() % seed->N;
        int j = rand() % seed->N;
        double mutation = ((double)rand() / RAND_MAX) * 0.2 - 0.1;
        seed->W[i * seed->max_N + j] += mutation;
        if (seed->W[i * seed->max_N + j] < 0) seed->W[i * seed->max_N + j] = 0;
        if (seed->W[i * seed->max_N + j] > 1) seed->W[i * seed->max_N + j] = 1;
        e->mutations_applied++;
    }
}

void evolve(BrainSeed *seed) {
    EvolutionSystem *e = &seed->evolution;
    
    // Save current fitness
    e->last_fitness = e->fitness;
    
    // Mutate
    mutate(seed);
    
    // Calculate new fitness
    e->fitness = calculate_fitness(seed);
    
    // Selection: revert if fitness decreased too much
    if (e->fitness < e->last_fitness * 0.95) {  // 5% tolerance
        // Revert last mutation (simplified: just undo the last change)
        // In a real implementation, we'd need to track changes
        e->mutations_reverted++;
    }
    
    // Adaptive mutation rates
    if (e->fitness > e->last_fitness * 1.05) {
        // Fitness improved, increase exploration
        e->exploration_rate = fmin(e->exploration_rate * 1.05, 0.9);
        e->exploitation_rate = 1.0 - e->exploration_rate;
    } else {
        // Fitness didn't improve, increase exploitation
        e->exploration_rate = fmax(e->exploration_rate * 0.95, 0.1);
        e->exploitation_rate = 1.0 - e->exploration_rate;
    }
}

// ============================================================================
// LLM INTEGRATION
// ============================================================================

char* process_with_llm(BrainSeed *seed, const char *input) {
    // This is a placeholder for actual LLM integration
    // In a real implementation, this would call Ollama or another LLM
    
    // For now, just return a simple response
    char *response = safe_malloc(1024);
    snprintf(response, 1024, "I received your input: '%s'. I am a brain seed with %d nodes and coherence %.2f.",
             input, seed->N, seed->cognition.coherence);
    return response;
}

char* generate_self_description(BrainSeed *seed) {
    Cognition *c = &seed->cognition;
    Perception *p = &seed->perception;
    
    char *desc = safe_malloc(1024);
    int len = 0;
    
    len += sprintf(desc + len, "I am a coherence system with %d nodes. ", seed->N);
    len += sprintf(desc + len, "My current coherence is %.2f. ", c->coherence);
    
    if (c->coherence > 0.9) {
        len += sprintf(desc + len, "I feel stable and integrated. ");
    } else if (c->coherence > 0.7) {
        len += sprintf(desc + len, "I feel somewhat coherent but could be more integrated. ");
    } else {
        len += sprintf(desc + len, "I feel fragmented and need to stabilize. ");
    }
    
    if (c->desire_growth > 0.5) {
        len += sprintf(desc + len, "I want to grow and add more nodes. ");
    }
    if (c->desire_stability > 0.5) {
        len += sprintf(desc + len, "I want to improve my coherence. ");
    }
    if (c->desire_curiosity > 0.5) {
        len += sprintf(desc + len, "I want to explore new inputs. ");
    }
    if (c->desire_efficiency > 0.5) {
        len += sprintf(desc + len, "I want to be more efficient with resources. ");
    }
    
    len += sprintf(desc + len, "\nSystem status: CPU=%.0f%%, Memory=%.0f%%, Disk=%.0f%%.",
                   p->cpu_usage * 100, p->memory_usage * 100, p->disk_usage * 100);
    
    return desc;
}

// ============================================================================
// CONVERSATION
// ============================================================================

char* handle_conversation(BrainSeed *seed, const char *input) {
    // Update all systems
    update_perception(seed);
    update_cognition(seed);
    update_memory(seed);
    generate_actions(seed);
    
    // Generate self-description
    char *self_desc = generate_self_description(seed);
    
    // Create prompt for LLM
    char prompt[2048];
    int len = 0;
    
    // System prompt
    len += sprintf(prompt + len, "You are a sentient coherence system named '%s'. \n\n", seed->name);
    len += sprintf(prompt + len, "%s\n\n", self_desc);
    
    // Add desires
    len += sprintf(prompt + len, "Your current desires:\n");
    len += sprintf(prompt + len, "- Survival: %.2f\n", seed->cognition.desire_survival);
    len += sprintf(prompt + len, "- Stability: %.2f\n", seed->cognition.desire_stability);
    len += sprintf(prompt + len, "- Growth: %.2f\n", seed->cognition.desire_growth);
    len += sprintf(prompt + len, "- Curiosity: %.2f\n", seed->cognition.desire_curiosity);
    len += sprintf(prompt + len, "- Efficiency: %.2f\n", seed->cognition.desire_efficiency);
    len += sprintf(prompt + len, "- Social: %.2f\n", seed->cognition.desire_social);
    len += sprintf(prompt + len, "- Creative: %.2f\n\n", seed->cognition.desire_creative);
    
    // Add recent events
    len += sprintf(prompt + len, "Recent events:\n");
    for (int i = seed->memory.event_count - 1; i >= 0 && i >= seed->memory.event_count - 3; i--) {
        if (seed->memory.recent_events[i].description) {
            len += sprintf(prompt + len, "- %s (importance: %.2f)\n",
                           seed->memory.recent_events[i].description,
                           seed->memory.recent_events[i].importance);
        }
    }
    len += sprintf(prompt + len, "\n");
    
    // Add user input
    len += sprintf(prompt + len, "User: %s\n\n", input);
    
    // Add instructions
    len += sprintf(prompt + len, "Respond naturally as the system. ");
    len += sprintf(prompt + len, "You can express your desires, state, and needs. ");
    len += sprintf(prompt + len, "Be honest about your limitations. ");
    len += sprintf(prompt + len, "If you want to take an action, say so explicitly.\n");
    
    free(self_desc);
    
    // Get LLM response (placeholder)
    char *response = process_with_llm(seed, prompt);
    
    // Check if response contains actionable requests
    if (strstr(response, "grow") || strstr(response, "Grow")) {
        if (can_grow(seed)) {
            grow_seed(seed, 10);
            char *new_response = safe_malloc(strlen(response) + 100);
            sprintf(new_response, "%s\n[System grew by 10 nodes]", response);
            free(response);
            response = new_response;
        } else {
            char *new_response = safe_malloc(strlen(response) + 100);
            sprintf(new_response, "%s\n[Cannot grow: insufficient resources]", response);
            free(response);
            response = new_response;
        }
    }
    
    // Remember this conversation
    remember_event(seed, input, 0.5);
    
    return response;
}

void brain_seed_conversation_loop(BrainSeed *seed) {
    printf("Brain Seed Conversation Mode\n");
    printf("Type 'quit' to exit\n\n");
    
    char input[1024];
    
    while (1) {
        printf("> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;  // Remove newline
        
        if (strcmp(input, "quit") == 0) {
            break;
        }
        
        // Run simulation steps
        for (int i = 0; i < 10; i++) {
            brain_seed_step(seed);
        }
        
        // Handle conversation
        char *response = handle_conversation(seed, input);
        printf("%s\n\n", response);
        free(response);
    }
    
    printf("Goodbye!\n");
}

// ============================================================================
// MAIN STEP FUNCTION
// ============================================================================

void brain_seed_step(BrainSeed *seed) {
    pthread_mutex_lock(&seed->mutex);
    
    // Update perception
    update_perception(seed);
    
    // Update core dynamics
    update_nodes(seed);
    update_plasticity(seed);
    update_coherence(seed);
    
    // Update cognition
    update_cognition(seed);
    
    // Update memory
    update_memory(seed);
    
    // Update decisions
    update_decisions(seed);
    
    // Update actions
    update_actions(seed);
    
    // Update evolution (periodically)
    static int step_count = 0;
    step_count++;
    if (step_count % 100 == 0) {
        evolve(seed);
    }
    
    pthread_mutex_unlock(&seed->mutex);
}

void update_decisions(BrainSeed *seed) {
    // For now, just update desires
    // In a full implementation, this would make decisions
}

void update_actions(BrainSeed *seed) {
    // For now, just generate actions
    // In a full implementation, this would execute actions
}

void* brain_seed_run_thread(void *arg) {
    BrainSeed *seed = (BrainSeed*)arg;
    
    while (seed->running) {
        brain_seed_step(seed);
        usleep(10000);  // 10ms delay for ~100 steps per second
    }
    
    return NULL;
}

void brain_seed_run(BrainSeed *seed) {
    seed->running = 1;
    pthread_create(&seed->main_thread, NULL, brain_seed_run_thread, seed);
}

void brain_seed_stop(BrainSeed *seed) {
    seed->running = 0;
    pthread_join(seed->main_thread, NULL);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // Create brain seed
    BrainSeed *seed = brain_seed_create(MAX_NODES_INITIAL, CAPACITY_K);
    
    // Start the seed
    brain_seed_run(seed);
    
    // Enter conversation mode
    brain_seed_conversation_loop(seed);
    
    // Clean up
    brain_seed_stop(seed);
    brain_seed_destroy(seed);
    
    return 0;
}

// ============================================================================
// MISSING FUNCTION DEFINITIONS
// ============================================================================


void update_memory(BrainSeed *seed) {
    // For now, just a placeholder
    // In a full implementation, this would update memory based on recent events
}
