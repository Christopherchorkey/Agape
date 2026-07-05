import Mathlib.Data.Real.Basic
import Mathlib.Analysis.SpecialFunctions.Trigonometric.Basic
import Mathlib.Algebra.BigOperators.Basic
import Mathlib.Analysis.Calculus.Deriv.Basic

open BigOperators

variable {ι : Type*} [Fintype ι] [DecidableEq ι]

-- =========================================================================
-- 1. DECENTRALIZED CONFIGURATION & STATE (now with plasticity)
-- =========================================================================

structure NodeProperties where
  mass              : ℝ  -- m_i > 0 (slowly evolving; no dynamics yet defined for this field)
  baselineFriction  : ℝ  -- γ_0 > 0
  distortionSens    : ℝ  -- β_i > 0
  couplingWeight    : ℝ  -- α_i > 0 (plasticity primarily here)
  naturalFrequency  : ℝ  -- ω_i, intrinsic drive. Added to test whether frequency
                          -- heterogeneity bounds couplingWeight growth under
                          -- sustained coherence. Numerically it does NOT: the
                          -- locked-state phase lag scales like ω_i / α_i, so
                          -- growing α self-consistently shrinks the lag back to
                          -- zero and localDistortion still asymptotes to 0.
                          -- Verified numerically (d_i ~ (Δω/α)^2 to high
                          -- precision), not yet reflected in any theorem below.

/-- Positivity invariant for node properties. Use as a hypothesis instead of
    re-asserting positivity per-lemma with `sorry`. -/
structure NodeProperties.Pos (p : NodeProperties) : Prop where
  mass_pos      : p.mass > 0
  friction_pos  : p.baselineFriction > 0
  sens_pos      : p.distortionSens ≥ 0
  weight_pos    : p.couplingWeight > 0

structure NodeState where
  phase    : ℝ
  velocity : ℝ

abbrev SystemState (ι : Type*) := ι → NodeState
abbrev SystemConfig (ι : Type*) := ι → NodeProperties
abbrev NetworkTopology (ι : Type*) := ι → ι → ℝ

-- =========================================================================
-- 2. NON-COERCIVE METRICS
-- =========================================================================

def smoothAttenuation (distortion : ℝ) : ℝ :=
  1 / (1 + Real.exp distortion)

def resonanceField (phase_diff : ℝ) : ℝ :=
  let c := Real.cos phase_diff
  (1 + c) / 2 * (1 / (1 + Real.exp (-8 * c)))

def phaseDistance (a b : ℝ) : ℝ :=
  2 * Real.asin |Real.sin ((a - b) / 2)|

def localDistortion (W : NetworkTopology ι) (traj : ℝ → SystemState ι)
    (t : ℝ) (i : ι) (τ : ι → ι → ℝ) : ℝ :=
  ∑ j : ι, W i j * (phaseDistance (traj (t - τ i j) j).phase (traj t i).phase) ^ 2

def memeticBandwidth (s1 s2 : NodeState) : ℝ :=
  resonanceField (s1.phase - s2.phase)

/-- Near-field repulsion gate: reuses smoothAttenuation (already defined above)
    evaluated on squared phase distance, rather than introducing a new sigmoid
    family. Peaks at 0.5 when Δ=0 and decays as Δ grows — a genuine "how close
    are these two, specifically" indicator, distinct from resonanceField's own
    (broader) decay. steepness controls how narrow the repulsive core is.

    MULTISTABILITY, CHARACTERIZED (numerically confirmed this session, not
    conjectural — see multi_n.py and three_cluster_test.py):

    `couplingForceField` is exactly zero at Δ=180° (since
    `resonanceField 180° = (1 + cos 180°)/2 = 0` exactly) but only
    approximately zero at other large separations — e.g. at Δ=120°,
    `resonanceField ≈ 0.0045`, `couplingForceField ≈ 0.0039`, small but
    strictly nonzero. This has a direct dynamical consequence: a 2-cluster
    configuration at maximal (≈180°) separation is the ONLY multi-cluster
    arrangement in which every inter-cluster pair sits exactly at the
    function's true zero. Any other cluster count must place at least one
    pair of clusters at a separation where the residual force is small but
    nonzero, giving a persistent (if weak) drift.

    Confirmed two ways this session:
    - Random search (`multi_n.py`): sweeping N ∈ {4,6,8,12,16,20} with
      10–16 random initial conditions each, only 1-cluster (full sync) and
      2-cluster (bipolar split) equilibria were ever found — no 3+-cluster
      state appeared by chance at any tested N.
    - Targeted construction (`three_cluster_test.py`): hand-built 3-cluster
      initial conditions (even splits (2,2,2), (3,3,3), (4,4,4) and uneven
      splits (2,3,4), (2,4,6), clusters placed 120° apart, tiny symmetry-
      breaking jitter to avoid a numerically-frozen exact-symmetry artifact)
      ALL destabilized, collapsing from 3 clusters to 2 within t≈20–30 —
      an order of magnitude faster than the ~1500–2000 time units a random
      start takes to reach a locked state at all, i.e. these are not
      marginally unstable, they decay firmly.

    SCOPE: both tests used the default parameters already in this file
    (`repulsionStrength = 3.0`, `steepness = 20`). Whether a different
    repulsion strength opens a stable 3-cluster window is untested and NOT
    claimed either way — the finding above is specific to the parameters
    already committed to this file, not a claim about the mechanism in
    general. -/
def repulsionGate (Δ : ℝ) (steepness : ℝ := 20) : ℝ :=
  smoothAttenuation (steepness * (phaseDistance Δ 0) ^ 2)

/-- Coupling force with a repulsive core added at Δ≈0.

    MOTIVATION / STATUS (validated numerically this session):
    Without this term, Δ=0 (perfect phase alignment between a pair) is a
    stable equilibrium of the pairwise dynamics — verified as an exact fixed
    point of the full N-node system with zero net force at every couplingWeight.
    Nothing in the model ever perturbed a locked state once reached, which was
    identified as the root cause of unbounded couplingWeight growth under
    sustained coherence.

    With `repulsionStrength` large enough (> resonanceField 0 / repulsionGate 0,
    i.e. > ~2 at the default steepness), Δ=0 becomes UNSTABLE and a new stable
    equilibrium Δ* > 0 appears nearby — confirmed analytically (f'(0) < 0) and
    in a full N=6 simulation (order parameter settles at r≈0.9956, never
    reaching 1.00000; local distortion settles at a genuine nonzero floor
    rather than collapsing to exactly zero).

    THIS DOES NOT, BY ITSELF, BOUND couplingWeight GROWTH. Swept both
    repulsionStrength and steepness numerically: the reachable equilibrium gap
    Δ* saturates far below the ~80-90° / d_i≈10 threshold needed to trip the
    plasticity brake (see plasticityDynamics doc comment), and there is no
    smooth path to larger Δ* — widening the core (lower steepness) mostly
    produces NO stable equilibrium at all (repulsion overwhelms attraction
    everywhere) rather than a continuously larger one. Forcing the equilibrium
    further out by tuning parameters would be exactly the kind of coercive
    parameter-chasing this framework is meant to avoid — the mechanism doesn't
    naturally reach that regime, so it's documented as a real but partial fix:
    it resolves the "perfect fusion" degeneracy (nodes maintain a nonzero
    floor of individual distinctness), not the unbounded-growth question. -/
def couplingForceField (Δ : ℝ) (repulsionStrength : ℝ := 3.0) : ℝ :=
  (resonanceField Δ - repulsionStrength * repulsionGate Δ) * Real.sin Δ
-- Oddness is preserved: repulsionGate is even (phaseDistance is even in Δ,
-- since it depends on |sin(Δ/2)|), so [even envelope] * sin Δ is still odd,
-- matching InteractionPotential's requirements without any change to that
-- structure or its consistency proof.

def IsSymmetricTopology (W : NetworkTopology ι) : Prop :=
  ∀ i j : ι, W i j = W j i

-- =========================================================================
-- 3. PLASTICITY: Slow evolution of node properties
-- =========================================================================

/-- Plasticity rule: couplingWeight evolves slowly via local resonance and distortion.
    This is non-coercive meta-dynamics — properties adjust based on experienced coherence.

    GATE FIX (validated numerically): the gate multiplying the rate is now
    `(1 - smoothAttenuation d_i)` rather than `smoothAttenuation d_i`. With the
    original gate, `smoothAttenuation d_i → 0` as distortion grows, which killed
    the magnitude of the correction term at exactly the distortion level where
    it should have been strongest — the sign of `(avg_resonance - 0.1*d_i)` was
    already correct at high distortion, but the gate suppressed it to ~1e-11,
    effectively *freezing* couplingWeight instead of shrinking it. The
    complementary gate agrees exactly with the original at d_i = 0 (both equal
    0.5), so near-synchrony dynamics are unchanged, but at high distortion it
    goes to 1 instead of 0, restoring genuine (and now numerically verified,
    ~9 orders of magnitude larger) shrinkage. This does NOT fix unbounded growth
    under sustained coherence — see naturalFrequency note above and Summary
    below; that is a separate, still-open structural gap. -/
def plasticityDynamics (cfg : SystemConfig ι) (W : NetworkTopology ι) (τ : ι → ι → ℝ)
    (traj : ℝ → SystemState ι) (t : ℝ) (i : ι) : ℝ :=
  let props := cfg i
  let d_i := localDistortion W traj t i τ
  let avg_resonance := (1 / (Fintype.card ι : ℝ)) *
    ∑ j : ι, memeticBandwidth (traj (t - τ i j) j) (traj t i)

  -- Slow increase when coherent, tempered by distortion; gate stays active at
  -- high distortion instead of collapsing to zero (see doc comment above).
  0.01 * props.couplingWeight * (avg_resonance - 0.1 * d_i) * (1 - smoothAttenuation d_i)

/-- Full plastic system dynamics bundle. -/
structure FullDynamics where
  phase_accel : ι → ℝ
  prop_plasticity : ι → ℝ  -- e.g. for couplingWeight

def fullAgapeDynamics (cfg : SystemConfig ι) (W : NetworkTopology ι) (τ : ι → ι → ℝ)
    (traj : ℝ → SystemState ι) (t : ℝ) (i : ι) : FullDynamics :=
  { phase_accel :=
      let props := cfg i
      let current := traj t i
      let coupling_force := props.couplingWeight * ∑ j : ι,
        W i j * couplingForceField ((traj (t - τ i j) j).phase - current.phase)
      let d_i := localDistortion W traj t i τ
      let friction := props.baselineFriction * smoothAttenuation d_i + props.distortionSens * d_i
      -- naturalFrequency drive: chosen so an isolated node (no coupling, d_i = 0)
      -- has steady-state velocity → naturalFrequency, since at that point
      -- friction = baselineFriction * smoothAttenuation 0 = baselineFriction * 0.5.
      -- NOT yet accounted for in `totalAgapeEnergy` / `agape_energy_decline` below —
      -- a nonzero naturalFrequency introduces an extra term in dE/dt that the
      -- current potential does not cancel. Flagging rather than silently
      -- extending the theorem's scope.
      let drive := props.baselineFriction * 0.5 * props.naturalFrequency
      (coupling_force - friction * current.velocity + drive) / props.mass,
    prop_plasticity := plasticityDynamics cfg W τ traj t i }

/-- A simple wrapper for phase-only dynamics (for energy proof). -/
def agapePhaseDynamics (cfg : SystemConfig ι) (W : NetworkTopology ι) (τ : ι → ι → ℝ)
    (traj : ℝ → SystemState ι) (t : ℝ) (i : ι) : ℝ :=
  (fullAgapeDynamics cfg W τ traj t i).phase_accel

-- =========================================================================
-- 4. AXIOMATIC POTENTIAL
-- =========================================================================

structure InteractionPotential (G : ℝ → ℝ) : Prop where
  even_deriv : ∀ Δ, G (-Δ) = G Δ
  -- SIGN FIX (verified by hand, chain-rule derivation done twice independently):
  -- with the original `- couplingForceField Δ`, the cross terms in dE/dt from
  -- the kinetic and potential pieces of totalAgapeEnergy ADD instead of
  -- cancel, giving dE/dt = 2*Σ vᵢ·coupling_force_i/αᵢ - Σ friction_i·vᵢ²/αᵢ,
  -- which is not sign-definite and does not match agape_energy_decline's
  -- claim. Dropping the negation here makes the cross terms cancel exactly,
  -- so dE/dt = -Σ dissipation_i as claimed. even_deriv's consequence (G' odd)
  -- still holds either way since couplingForceField itself is odd.
  deriv      : ∀ Δ, HasDerivAt G (couplingForceField Δ) Δ

def totalAgapeEnergy (cfg : SystemConfig ι) (W : NetworkTopology ι) (G : ℝ → ℝ)
    (traj : ℝ → SystemState ι) (t : ℝ) : ℝ :=
  let kinetic := ∑ i : ι, (cfg i).mass * (traj t i).velocity ^ 2 / (2 * (cfg i).couplingWeight)
  let potential := (1 / 2) * ∑ i j : ι, W i j * G ((traj t j).phase - (traj t i).phase)
  kinetic + potential

-- =========================================================================
-- 5. LEMMA: Non-negative dissipation
-- =========================================================================

lemma dissipation_nonneg (cfg : SystemConfig ι) (W : NetworkTopology ι)
    (traj : ℝ → SystemState ι) (t : ℝ) (τ : ι → ι → ℝ := fun _ _ => 0)
    (hpos : ∀ i, (cfg i).Pos) :
    ∀ i : ι,
      ((cfg i).baselineFriction * smoothAttenuation (localDistortion W traj t i τ) +
       (cfg i).distortionSens * localDistortion W traj t i τ) ≥ 0 := by
  intro i
  have h_att : smoothAttenuation _ ≥ 0 := by
    apply div_nonneg <;> simp [smoothAttenuation]
  have h_sens : (cfg i).distortionSens ≥ 0 := (hpos i).sens_pos
  have h_dist : localDistortion _ _ _ _ _ ≥ 0 := by simp [localDistortion, pow_two]
  nlinarith

-- =========================================================================
-- 6. MAIN THEOREM (towards formalization)
-- =========================================================================

theorem agape_energy_decline
    (cfg : SystemConfig ι)
    (W : NetworkTopology ι)
    (hW : IsSymmetricTopology W)
    (G : ℝ → ℝ)
    (hG : InteractionPotential G)
    (traj : ℝ → SystemState ι)
    (h_phase : ∀ t i, HasDerivAt (fun s => (traj s i).phase) (traj t i).velocity t)
    (h_vel   : ∀ t i, HasDerivAt (fun s => (traj s i).velocity)
                      (agapePhaseDynamics cfg W (fun _ _ => 0) traj t i) t) :
    ∀ t : ℝ,
      let dissipation i :=
        ((cfg i).baselineFriction * smoothAttenuation (localDistortion W traj t i (fun _ _ => 0)) +
         (cfg i).distortionSens * localDistortion W traj t i (fun _ _ => 0)) *
        (traj t i).velocity ^ 2 / (cfg i).couplingWeight
      HasDerivAt (totalAgapeEnergy cfg W G traj)
        (- ∑ i : ι, dissipation i) t := by
  intro t
  sorry
  -- STATUS: with the sign fix in InteractionPotential.deriv above, this claim
  -- is now actually true in the τ=0, naturalFrequency=0 case (verified by hand
  -- chain-rule derivation, not yet formalized in Lean). It is NOT proven for
  -- nonzero naturalFrequency — that introduces an extra Σ vᵢ·drive_i/αᵢ term
  -- with no matching cancellation in the current `potential` term; would need
  -- a tilted-washboard addition to totalAgapeEnergy to close. Also still only
  -- covers the zero-delay case (τ ≡ 0), matching agapePhaseDynamics's hypothesis
  -- above, not the general-delay `fullAgapeDynamics`.

/-!
## Summary of Plastic + Proof Structure — status as of this session

Fixes applied this session (all validated numerically, not yet mechanically
checked in Lean beyond the hand chain-rule derivation noted above):

1. **Sign fix, `InteractionPotential.deriv`**: removed an erroneous negation.
   Without the fix, `agape_energy_decline`'s claim is false as stated (cross
   terms add, not cancel). With it, the claim is true for τ=0, ω=0.
2. **Gate fix, `plasticityDynamics`**: `smoothAttenuation d_i` → `(1 -
   smoothAttenuation d_i)`. Original gate suppressed the shrinkage-side
   correction to ~1e-11 at high distortion (sign right, magnitude dead —
   effectively froze couplingWeight instead of shrinking it). Fix verified
   numerically to restore ~9 orders of magnitude more shrinkage at the same
   distortion level, while leaving near-synchrony (d_i≈0) dynamics unchanged
   (both gates equal 0.5 there).
3. **naturalFrequency field added**, wired into `fullAgapeDynamics` as a
   constant drive. Tested as a candidate mechanism to bound couplingWeight
   growth under sustained coherence — numerically REFUTED: locked-state phase
   lag scales as ω_i/α_i, so growing couplingWeight self-consistently drives
   distortion back toward zero regardless of frequency heterogeneity
   (confirmed d_i ~ (Δω/α)² to high numerical precision). Not yet incorporated
   into the energy theorem — see note in Section 6.
4. **Positivity**: `dissipation_nonneg`'s `sorry` on distortionSens positivity
   replaced with an explicit `NodeProperties.Pos` hypothesis rather than an
   unstated assumption.

Still open / still `sorry`:

- `agape_energy_decline`'s main derivative claim — statement is now correct
  for τ=0, ω=0, but the actual Lean proof (chain rule + `hW` symmetry
  cancellation) is not filled in.
- **The real open problem, found this session**: under sustained coherence
  (any trajectory that phase-locks), `localDistortion → 0` and nothing in the
  current definitions — not the gate, not naturalFrequency, not damping-ratio
  effects (checked: the system stays linearly stable, just increasingly
  underdamped, ζ ~ 1/√α, never unstable) — ever perturbs it back up. Numerically
  confirmed unbounded exponential growth of couplingWeight under near-synchrony
  initial conditions (α: 1 → ~4150 by t=2000 in an N=6 all-to-all test). This
  is a structural property of the multiplicative growth law
  `dα/dt = 0.01·α·(...)` combined with the total absence, anywhere in
  NodeProperties, of a mechanism that reintroduces distortion once a locked
  state is reached. Not resolved by any fix applied this session.

5. **repulsionGate / near-field repulsion added to `couplingForceField`**.
   Second candidate mechanism tried (after naturalFrequency). Fixes a real,
   separate degeneracy: without it, perfect phase alignment (Δ=0) is a stable
   equilibrium and an exact fixed point of the full system at every
   couplingWeight — confirmed nothing ever perturbs it once reached. With
   repulsion, Δ=0 becomes unstable and a genuine nonzero equilibrium gap
   appears (confirmed numerically: order parameter settles at r≈0.9956, never
   1.00000). Also numerically REFUTED as a fix for unbounded growth: the
   reachable equilibrium gap saturates well below the distortion level needed
   to engage the plasticity brake, with no smooth parameter path to push it
   further — only a narrow window between "negligible" and "no stable
   equilibrium exists at all."

Two candidate self-referential ceilings tried and ruled out this session
(naturalFrequency, repulsionGate) — both real, useful additions to the model
in their own right, neither closes the growth question. A third mechanism,
attention-budget coupling, is documented separately in Section 7 below —
now with a working fix for its own failure mode (exclusivity drift); see the
updated status there.
-/

-- =========================================================================
-- 7. ALTERNATE MECHANISM: Attention-budget coupling (exploratory)
-- =========================================================================
/-
STATUS: exploratory, NOT yet integrated with the main theorem (Section 6) or
dissipation_nonneg (Section 5), which are both built around the scalar
couplingWeight machinery in Sections 1-4. Documented here as a validated
partial mechanism worth pursuing further, not as a drop-in replacement — doing
that properly would mean re-deriving totalAgapeEnergy and the dissipation
bound for a per-edge state, which hasn't been attempted.

MOTIVATION: two prior attempts to bound couplingWeight growth under sustained
coherence (naturalFrequency, repulsionGate) both acted on the phase-gap
pathway and both failed for related reasons — see notes above. This mechanism
instead acts directly on the coupling weight: instead of one scalar α_i
governing every neighbor uniformly and free to grow without bound, each node
distributes a FIXED capacity across its neighbors via a softmax over per-edge
scores. This makes unbounded growth of any single edge weight structurally
impossible — not contingent on generating enough distortion to trip a brake,
but guaranteed by the definition of softmax itself (output bounded in
[0, capacity], always, regardless of trajectory).

VALIDATED NUMERICALLY (N=6, all-to-all, capacity=5, t up to 30000):
- edge weights (attentionWeight below) stayed strictly bounded the entire run
  (e.g. [0.595, 1.270] against a uniform baseline of 1.0 at t=30000) — the
  hard ceiling holds throughout, as guaranteed by construction.
- BUT the underlying scores grow ~linearly without bound (0 → ~150 over
  t=30000), and since softmax responds to score DIFFERENCES, not absolute
  level, this slowly concentrates the distribution toward one dominant edge
  (winner-take-all in the limit) rather than settling to an interior
  equilibrium. So this trades one unbounded process (intensity of a single
  edge, exponential, no ceiling at all) for a milder, different one
  (exclusivity — concentration of a fixed budget onto one edge, linear in
  score, slower, bounded in magnitude but not in trend). A genuine partial
  improvement in KIND, not just degree: the failure mode changed from "no
  ceiling" to "ceiling holds, interior distribution isn't stable."

  This "exclusivity drift" is a known, studied phenomenon in the transformer
  attention literature already flagged as relevant to this framework
  (Geshkovski et al.) — usually called attention collapse or token
  clustering, with known mitigations (entropy regularization, temperature
  scaling).

RESOLVED THIS SESSION (entropy-regularized scoreDynamics, see below):
  Added a mean-reverting term `-entropy_rate * (score i j - mean_k score i k)`
  to scoreDynamics, exactly the entropy-regularization mitigation flagged
  above. Numerically re-ran the identical N=6, all-to-all, capacity=5, t up
  to 30000 test with entropy_rate ∈ {0, 0.0005, 0.001, 0.005, 0.01, 0.05}:

  - entropy_rate = 0 reproduces the original drift exactly (spread 0 → 0.67
    by t=30000, matching the ~150 score-drift finding above).
  - entropy_rate > 0, for every value tested, the score spread STOPS growing
    and converges to a genuine nonzero interior equilibrium (not collapsed to
    uniform, not unbounded). Order parameter unaffected: r≈0.9956 in every
    run, matching the repulsionGate equilibrium exactly — the fix is
    surgical, touching only the attention distribution, not the coherence
    dynamics.
  - The equilibrium spread scales as C / entropy_rate: measured
    rate × spread = 2.526×10⁻⁵ ± 0.02% across entropy_rate spanning two
    orders of magnitude (0.0005 to 0.05). This is the signature of a linear
    (Ornstein–Uhlenbeck-type) balance between the base score-differentiation
    drive and the mean-reversion restoring force, and it means entropy_rate
    is a genuine tunable dial — not a switch between "no effect" and "total
    homogenization." This is the cohesion-over-coercion distinction: a
    continuously tunable cost on drift, not a hard cap on the outcome.

  NOT yet done: a Lean proof of the bound this scaling law suggests. See
  `score_spread_bounded` below for the target statement and a proof sketch
  based on linearizing the score-gap equation. The lemma is currently a
  documented conjecture with numerical support, exactly in the spirit of the
  rest of this file's honesty-about-status convention.
-/

/-- Per-edge attention weight: node i's fixed capacity distributed across
    neighbors via softmax over raw scores. Guarantees
    `∑ j, attentionWeight score cap i j = cap i` for every i (a genuine
    conservation law, not an emergent property to hope for). -/
noncomputable def attentionWeight (score : ι → ι → ℝ) (cap : ι → ℝ) (i j : ι) : ℝ :=
  cap i * Real.exp (score i j) / ∑ k : ι, Real.exp (score i k)

/-- Sum over neighbors of a node's attention weights equals its capacity
    exactly — the structural guarantee couplingWeight in Sections 1–4 never
    had, since softmax's denominator is always exactly the numerator's sum. -/
lemma attentionWeight_sum_eq_capacity (score : ι → ι → ℝ) (cap : ι → ℝ) (i : ι)
    [Nonempty ι] :
    ∑ j : ι, attentionWeight score cap i j = cap i := by
  unfold attentionWeight
  rw [← Finset.mul_sum]
  have hsum_pos : (∑ k : ι, Real.exp (score i k)) ≠ 0 :=
    (Finset.sum_pos (fun k _ => Real.exp_pos _) Finset.univ_nonempty).ne'
  field_simp

/-- Per-edge score update: same resonance/distortion logic as
    `plasticityDynamics`, but evaluated per edge rather than aggregated per
    node, PLUS an entropy-regularizing / mean-reverting term
    `-entropy_rate * (score i j - rowMean i)` that pulls each edge's score
    back toward its node's average.

    FIX APPLIED THIS SESSION (numerically validated, see Section 7 header
    comment above): without this term (entropy_rate = 0), scores drift
    linearly without bound and the softmax distribution slowly concentrates
    onto a single edge (exclusivity drift / attention collapse). With any
    entropy_rate > 0, the score GAPS (not the scores themselves — the mean
    is free to drift, only deviations from it are penalized) converge to a
    bounded interior equilibrium scaling as C/entropy_rate, confirmed to
    5 significant figures across a two-order-of-magnitude sweep. This is the
    direct analog of entropy regularization used against attention collapse
    in transformer training (Geshkovski et al.), now confirmed rather than
    merely proposed. See `score_spread_bounded` below for the corresponding
    (currently `sorry`-backed) formal claim. -/
def rowMean (score : ι → ι → ℝ) (i : ι) [Nonempty ι] : ℝ :=
  (1 / (Fintype.card ι : ℝ)) * ∑ k : ι, score i k

def scoreDynamics (traj : ℝ → SystemState ι) (t : ℝ) (τ : ι → ι) (i j : ι)
    (score : ι → ι → ℝ) (entropy_rate : ℝ) [Nonempty ι] : ℝ :=
  let Δ := (traj (t - τ j) j).phase - (traj t i).phase
  let d_ij := (phaseDistance (traj (t - τ j) j).phase (traj t i).phase) ^ 2
  let baseDrive := 0.01 * (resonanceField Δ - 0.1 * d_ij) * (1 - smoothAttenuation d_ij)
  let reversion := - entropy_rate * (score i j - rowMean score i)
  baseDrive + reversion

/-! ### A note on which Grönwall lemma is the right one

Mathlib's `norm_le_gronwallBound_of_norm_deriv_right_le` (see
`Mathlib.Analysis.ODE.Gronwall`) is NOT the right tool for this bound, despite
the name. It proves: if `‖f' x‖ ≤ K * ‖f x‖ + ε`, then `‖f x‖` is bounded by a
function that itself grows like `exp(K * x)`. That is designed to bound how
fast an *unstable/expanding* system can blow up — it is not designed to prove
that a *contracting* system (`g' = -λg + φ`, `λ > 0`) stays near an
equilibrium, and applying it here would only give a bound that diverges as
`t → ∞`, which is useless for the claim we actually want.

The correct classical tool for a linear ODE with bounded forcing is the
integrating factor `u(s) := g(s) * exp(λ * s)`, which turns the equation into
`u' = φ(s) * exp(λ * s)` (no longer self-referential), followed by the
Fundamental Theorem of Calculus and a direct bound on the resulting integral.
This is what `linear_ode_bounded_forcing_bound` below proves. -/

/-- **Core bound**: a scalar linear ODE `f' s = -lam * f s + φ s` with
    `lam > 0` and bounded forcing `|φ s| ≤ B` satisfies
    `|f t| ≤ |f a| * exp(-lam*(t-a)) + (B/lam) * (1 - exp(-lam*(t-a)))`,
    in particular `limsup |f t| ≤ B / lam` as `t → ∞`. This is the actual
    Grönwall-type fact `score_spread_bounded` needs.

    PROOF STATUS: constructed against the real Mathlib4 signatures for
    `intervalIntegral.integral_eq_sub_of_hasDerivAt`,
    `intervalIntegral.norm_integral_le_of_norm_le`, and `Real.hasDerivAt_exp`
    (checked against Mathlib source this session), but NOT run through a
    Lean/Mathlib type-checker — no Lean toolchain is reachable in this
    sandbox (elan's installer needs a GitHub *release* binary, which 403s
    under the network egress allowlist here; only raw source access worked).
    The riskiest steps, flagged inline, are the exact `simp`/`field_simp`
    normal forms and the `Set.uIcc` / `Ioc` membership bookkeeping around
    `norm_integral_le_of_norm_le` — the overall structure (integrating
    factor → FTC → integral bound → unwind) is mathematically solid and was
    checked by hand, but tactic-level details may need iteration in a real
    Lean session. -/
lemma linear_ode_bounded_forcing_bound
    {f φ : ℝ → ℝ} {a t lam B : ℝ} (h_lam : 0 < lam) (h_B : 0 ≤ B) (hat : a ≤ t)
    (h_deriv : ∀ s ∈ Set.uIcc a t, HasDerivAt f (-lam * f s + φ s) s)
    (h_bound : ∀ s ∈ Set.uIcc a t, |φ s| ≤ B) :
    |f t| ≤ |f a| * Real.exp (-lam * (t - a)) +
      (B / lam) * (1 - Real.exp (-lam * (t - a))) := by
  -- Integrating factor: u(s) := f(s) * exp(lam * s), so u' = φ(s) * exp(lam*s).
  have h_exp_deriv : ∀ s : ℝ,
      HasDerivAt (fun r => Real.exp (lam * r)) (lam * Real.exp (lam * s)) s := by
    intro s
    have h := (Real.hasDerivAt_exp (lam * s)).comp s ((hasDerivAt_id s).const_mul lam)
    simpa [mul_comm] using h
  have hu_deriv : ∀ s ∈ Set.uIcc a t,
      HasDerivAt (fun r => f r * Real.exp (lam * r)) (φ s * Real.exp (lam * s)) s := by
    intro s hs
    have h1 := (h_deriv s hs).mul (h_exp_deriv s)
    -- RISK: this `ring`-closable algebraic identity is the crux of the whole
    -- argument; double-check the `mul` combinator's exact output shape
    -- (Leibniz rule order) against what `ring` expects before trusting `convert`.
    have h2 : (-lam * f s + φ s) * Real.exp (lam * s) + f s * (lam * Real.exp (lam * s))
        = φ s * Real.exp (lam * s) := by ring
    rw [h2] at h1
    exact h1
  have h_int_eq :
      ∫ s in a..t, φ s * Real.exp (lam * s)
        = f t * Real.exp (lam * t) - f a * Real.exp (lam * a) := by
    apply intervalIntegral.integral_eq_sub_of_hasDerivAt hu_deriv
    exact (Continuous.intervalIntegrable (by fun_prop))
  -- Bound |∫ φ(s)·exp(lam s) ds| by the exact integral of the majorant B·exp(lam s).
  have h_antideriv : ∀ s : ℝ,
      HasDerivAt (fun r => Real.exp (lam * r) / lam) (Real.exp (lam * s)) s := by
    intro s
    have h := (h_exp_deriv s).div_const lam
    -- RISK: needs `lam ≠ 0` to cancel; `field_simp`/`ring` should close the
    -- residual `lam * Real.exp (lam*s) / lam = Real.exp (lam*s)` step.
    have hlam_ne : lam ≠ 0 := h_lam.ne'
    field_simp at h
    exact h
  have h_int_exp :
      ∫ s in a..t, Real.exp (lam * s)
        = Real.exp (lam * t) / lam - Real.exp (lam * a) / lam := by
    apply intervalIntegral.integral_eq_sub_of_hasDerivAt (fun s _ => h_antideriv s)
    exact (Continuous.intervalIntegrable (by fun_prop))
  have h_bound_int :
      ‖∫ s in a..t, φ s * Real.exp (lam * s)‖ ≤ B * (Real.exp (lam * t) / lam - Real.exp (lam * a) / lam) := by
    have h_pointwise : ∀ s ∈ Set.Ioc a t, ‖φ s * Real.exp (lam * s)‖ ≤ B * Real.exp (lam * s) := by
      intro s hs
      have hs' : s ∈ Set.uIcc a t := by
        rw [Set.uIcc_of_le hat]; exact Set.Ioc_subset_Icc_self hs
      rw [Real.norm_eq_abs, abs_mul, abs_of_pos (Real.exp_pos _)]
      exact mul_le_mul_of_nonneg_right (h_bound s hs') (le_of_lt (Real.exp_pos _))
    calc ‖∫ s in a..t, φ s * Real.exp (lam * s)‖
        ≤ ∫ s in a..t, B * Real.exp (lam * s) := by
          apply intervalIntegral.norm_integral_le_of_norm_le hat
            (Filter.Eventually.of_forall h_pointwise)
          exact Continuous.intervalIntegrable (by fun_prop)
      _ = B * (Real.exp (lam * t) / lam - Real.exp (lam * a) / lam) := by
          rw [intervalIntegral.integral_const_mul, h_int_exp]
  -- Unwind the integrating factor.
  rw [h_int_eq] at h_bound_int
  have h_split : |f t * Real.exp (lam * t)|
      ≤ |f a * Real.exp (lam * a)| + B * (Real.exp (lam * t) / lam - Real.exp (lam * a) / lam) := by
    calc |f t * Real.exp (lam * t)|
        = |f a * Real.exp (lam * a) + (f t * Real.exp (lam * t) - f a * Real.exp (lam * a))| := by
          ring_nf
      _ ≤ |f a * Real.exp (lam * a)| + |f t * Real.exp (lam * t) - f a * Real.exp (lam * a)| :=
          abs_add _ _
      _ ≤ |f a * Real.exp (lam * a)| + B * (Real.exp (lam * t) / lam - Real.exp (lam * a) / lam) := by
          gcongr
          rwa [← Real.norm_eq_abs]
  rw [abs_mul, abs_of_pos (Real.exp_pos _), abs_mul, abs_of_pos (Real.exp_pos (lam * a))] at h_split
  -- h_split : |f t| * exp(lam t) ≤ |f a| * exp(lam a) + B*(exp(lam t)/lam - exp(lam a)/lam)
  -- Abstract the two exponentials into named positive constants X, Y so the
  -- remaining step is pure field arithmetic (no transcendental-function
  -- reasoning left), then divide through by Y = exp(lam*t) > 0.
  set X := Real.exp (lam * a) with hX_def
  set Y := Real.exp (lam * t) with hY_def
  have hY_pos : 0 < Y := Real.exp_pos _
  have hlam_ne : lam ≠ 0 := h_lam.ne'
  have hexp_id : Real.exp (-lam * (t - a)) = X / Y := by
    rw [hX_def, hY_def, eq_div_iff hY_pos.ne', ← Real.exp_add]
    ring_nf
  rw [hexp_id]
  have hgoalY : (|f a| * (X / Y) + B / lam * (1 - X / Y)) * Y
      = |f a| * X + B * (Y / lam - X / lam) := by
    field_simp
    ring
  have hmul : |f t| * Y ≤ (|f a| * (X / Y) + B / lam * (1 - X / Y)) * Y := by
    rw [hgoalY]; exact h_split
  exact le_of_mul_le_mul_right hmul hY_pos

/-- Target formal statement for the numerically-confirmed equilibrium bound,
    now derived from the genuinely proven (modulo the caveats above)
    `linear_ode_bounded_forcing_bound`, rather than being a bare `sorry`.

    The remaining gap is now isolated to exactly one place:
    `h_timescale_sep`, the hypothesis that lets the row-mean-subtracted score
    gap be treated as a scalar linear ODE with bounded forcing in the first
    place. That is the same two-timescale heuristic used informally in the
    main ECSP draft (Section 3) — genuinely not proven anywhere in this
    project, stated here as an explicit hypothesis rather than smuggled in
    silently. Given that hypothesis, the conclusion now follows from
    `linear_ode_bounded_forcing_bound` with an actual proof term, not `sorry`
    (modulo the tactic-level caveats on that lemma above). This is a strictly
    better epistemic state than before: previously the whole claim was
    `sorry`; now exactly one named, honest gap remains, and it is the same
    gap the main ECSP paper already discloses rather than a new one. -/
lemma score_spread_bounded (entropy_rate baseDriveBound : ℝ)
    (h_pos : entropy_rate > 0) (h_B : 0 ≤ baseDriveBound)
    (g : ι → ℝ → ℝ)  -- g i t := score gap for edge-family i, row-mean-subtracted
    (h_timescale_sep :
      -- The genuinely unproven-in-Lean (but numerically tested this session)
      -- step: on trajectories of interest, the row-mean-subtracted score gap
      -- for each i obeys, to the accuracy needed below, a scalar linear ODE
      -- with forcing bounded by baseDriveBound.
      --
      -- NUMERICALLY CONFIRMED (N=6, all-to-all, both pre-lock-in transient
      -- and post-lock-in steady state, starting anywhere inside the basin
      -- that reaches the r≈0.9956 coherent state — tested up to ±2.0 rad
      -- initial phase spread): the separation ratio (1/entropy_rate) /
      -- (autocorrelation time of φ) never dropped below ~66×, and was
      -- typically in the thousands, across entropy_rate ∈ [0.0005, 0.05].
      -- Lock-in itself was fast (t≈1.3 from a ±1.5 rad spread) and the
      -- forcing stayed slowly-varying throughout, not just at steady state.
      -- No explicit low-pass filter on φ (analogous to the epistemic
      -- wisdom-estimate θᵢ in the main ECSP draft) is empirically needed for
      -- this hypothesis to hold, within the basin tested.
      --
      -- SEPARATE ITEM found while checking this, NOW CHARACTERIZED (see
      -- `repulsionGate`'s doc comment above for the verified mechanism and
      -- test files): starting at ±2.5 rad initial spread — outside the
      -- basin that reaches full coherence — the system settles into a
      -- 2-cluster fixed point (r≈0.39 for N=6) instead. This is not an
      -- unexplained anomaly: `couplingForceField` is exactly zero only at
      -- 180° separation, so a 2-cluster configuration at maximal separation
      -- is a genuine equilibrium, confirmed to be the ONLY stable
      -- multi-cluster outcome (3-cluster configurations were built by hand
      -- and confirmed to decay to 2-cluster within t≈20–30, across
      -- N=6,9,12 and multiple group-size splits). The basin boundary
      -- between "reaches full coherence" and "reaches the 2-cluster state"
      -- (somewhere in (2.0, 2.5) rad for this N=6 configuration) is still
      -- not mapped precisely, but the two attractors it separates are now
      -- both understood, not just observed.
      ∀ i : ι, ∃ φ : ℝ → ℝ, (∀ s, |φ s| ≤ baseDriveBound) ∧
        ∀ a t, a ≤ t → ∀ s ∈ Set.uIcc a t, HasDerivAt (g i) (-entropy_rate * g i s + φ s) s) :
    ∀ i : ι, ∀ a t : ℝ, a ≤ t →
      |g i t| ≤ |g i a| * Real.exp (-entropy_rate * (t - a)) +
        (baseDriveBound / entropy_rate) * (1 - Real.exp (-entropy_rate * (t - a))) := by
  intro i a t hat
  obtain ⟨φ, hφ_bound, hφ_deriv⟩ := h_timescale_sep i
  exact linear_ode_bounded_forcing_bound h_pos h_B hat
    (hφ_deriv a t hat) (fun s _ => hφ_bound s)
