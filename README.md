
# Bionetta Ultra-Groth

This is case-specific implementation of Ultra-Groth protocol based on Rapidsnark prover. It's generates proofs for circuits created in [Bionetta SDK](https://github.com/rarimo/bionetta) pretty fast. Check out technical [REAMDE](./src/README.md) if you curios about it.

## Dependencies

You should have installed gcc, cmake, libsodium, and gmp (development)

In ubuntu:

```
sudo apt-get install build-essential cmake libgmp-dev libsodium-dev nasm curl m4
```

On MacOS:

```
brew install cmake gmp libsodium nasm
```

## Compile prover in standalone mode

### Compile prover for x86_64 host machine

```sh
git submodule init
git submodule update
./build_gmp.sh host
make host
```

### Compile prover for macOS arm64 host machine

```sh
git submodule init
git submodule update
./build_gmp.sh macos_arm64
make macos_arm64
```

### Compile prover for linux arm64 host machine

```sh
git submodule init
git submodule update
./build_gmp.sh host
make host_arm64
```

### Compile prover for linux arm64 machine

```sh
git submodule init
git submodule update
./build_gmp.sh host
make arm64
```

## Building proof

You have a full prover compiled in the build directory.

So you can replace snarkjs command:

```sh
snarkjs groth16 prove <circuit.zkey> <witness.wtns> <proof.json> <public.json>
```

by this one
```sh
./build_prover/src/prover_ultra_groth <circuit.zkey> <witness.uwtns> <proof.json> <public.json>
```

### IMPORTANT NOTE

This implementation works correctly ONLY with circuits compiled in Bionetta SDK infrastructure - `.zkey` should be generated using our [SnarkJS](https://github.com/rarimo/ultragroth-snarkjs/tree/dev) fork and witness files from our [custom witness gen](https://github.com/rarimo/bionetta-witness-generator);

Use 

```sh
./build_prover/src/verifier_ultra_groth <vkey.json> <public.json> <proof.json>
```

to verify proof.

## Benchmarks

This prover parallelizes as much as it can the proof generation. Note, that performance difference between our implementation and Rapidsnark comes down to lookup tables, which reduce constraints and witness size of circuit rather than "bare-metal" optimizations.

## Proving Time

| Model   | Ultra-Groth (s) | Groth16 (s) | EZKL (s) | ZKML (s) | Deep-prove (s) | Params    |
|---------|-----------------|-------------|----------|----------|----------------|-----------|
| Model1  | 0.57            | 0.12        | 21.66    | 25.00    | 0.378          | 79,510    |
| Model2  | 0.73            | 0.27        | 656.21   | 232.55   | 1.989          | 2,006,020 |
| Model3  | 0.74            | 2.19        | 44.88    | 209.13   | 0.896          | 49,870    |
| Model4  | 1.08            | 2.20        | 163.50   | 220.84   | 1.010          | 133,870   |
| Model5  | 0.89            | 2.22        | 332.20   | 222.81   | 1.557          | 996,110   |
| Model6  | 1.79            | 4.72        | 663.93   | 230.72   | 2.516          | 1,974,070 |

### Annotation

Demonstrated models have different number of activations (increasing with model's numeral).