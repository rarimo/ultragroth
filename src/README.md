# General Info

This implementation is a 2-round variation of the Ultra-Groth protocol. It's highly specific and currently functions correctly only with circuits compiled using the Bionetta SDK.

---

# Technical Details

See *(paste write-up here)* if you're curious about how Ultra-Groth works.

This implementation is designed to be used alongside the Bionetta witness generator (referred to as the *Rust-side* hereinafter). Once you've defined your circuit, the Rust-side provides the indexes of signals that belong to different rounds of the protocol, along with their internal positions in Rust. These indexes are also passed to our fork of SnarkJS to compute the trusted setup.

Afterward, the Rust-side fills the signals related to the first round and writes them to a `.uwtns` file. This file includes the following:

- **Header** – Contains version, number of sections, and total number of signals in the witness.
- **Signals** – The full witness vector (with zeros for signals belonging to the second round).
- **Chunks** – Chunks computed during the first round of the protocol.
- **Frequencies** – Corresponding frequencies of the chunks.
- **Push Indexes** – Internal Rust positions of the second-round-related signals (this section appears last but is used to define the next one).
- **Witness Indexes** – Indexes of values (located at *push indexes*) inside the witness vector.

When `.uwtns` is passed to the prover:

1. The first-round commitment is calculated using the relevant signals from the witness vector.
2. Randomness is derived using the Fiat-Shamir transform.
3. Using the chunks, frequencies, and provided indexes, the C++ prover fills the second-round signals and computes the final commitment.

---

# Proof Format

The resulting proof contains:

- 3 points similar to vanilla Groth16,
- 1 point for each executed round (1 in our case),
- Public parts of the witness for the verifier.

The trusted setup includes a separate field for the commitment related to signals from the derived randomness. The randomness itself is not stored in the public inputs, as the verifier derives it from the commitments provided by the prover.

---

# Comparison with Vanilla Groth16

This comparison is specific to the circuits used in this context.

In the Bionetta implementation of activation functions (a major source of constraints), we significantly reduce the number of constraints and witness elements by using lookup tables. For the `zkLiveness` model:

- The witness is ~4× smaller compared to a Groth16-based implementation.
- However, performance gains are not linear. Activation functions based on bit decomposition produce many constraints, but the corresponding witness elements are mostly zeros — this simplifies MSM (multi-scalar multiplication) calculations.
- Conversely, the chunk signals produced by lookups contain far more non-zero values, so MSM execution time remains comparable to that in the Groth16 circuit.
- FFT execution (the most computationally expensive part of the proving process) is significantly faster due to the smaller circuit size, which results in greatly reduced overall proving time.
