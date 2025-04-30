#!/bin/bash

# Prove
./build_prover/src/prover_ultra_groth liveness_final.zkey input_zk_liveness.json

# Verify
./build_prover/src/verifier_ultra_groth liveness_vkey.json public_inputs.json proof.json