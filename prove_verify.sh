#!/bin/bash

# Prove
./build_prover/src/prover_ultra_groth seheavy_lookup_final.zkey input_seheavy.json seheavy_lookup.sym

# Verify
./build_prover/src/verifier_ultra_groth seheavy_lookup.vkey.json input_seheavy_verifier.json proof.json