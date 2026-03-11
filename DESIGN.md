# Design Notes

## Pipeline

The compiler lowers programs through the following chain:

```text
LB -> LA -> IR -> L3 -> L2 -> L1 -> x86-64
```

Each compiler stage removes one layer of abstraction:

- `LBc`: lowers high-level control flow and declaration structure
- `LAc`: inserts value encoding and access checks
- `IRc`: performs optimization and IR lowering
- `L3c`: performs instruction selection
- `L2c`: performs register allocation
- `L1c`: emits `x86-64` assembly

## Why Multiple Stages

This repository is intentionally structured as a staged compiler rather than a single direct `LB -> x86-64` translator.

That design makes the system easier to:

- implement incrementally
- debug by inspecting intermediate outputs
- extend without mixing unrelated invariants in one pass

## Reproducibility

The intended reproducible workflow is:

```sh
docker compose -f docker/docker-compose.yml up -d --build
docker compose -f docker/docker-compose.yml exec dev bash
make
make test
make benchmark
```

## User Programs

User-authored programs live under:

```text
programs/<stage>/scratch/
```

The root `Makefile` exposes `exec_*` and `run_*` targets for each stage. Generated executables are written under:

```text
build/programs/<stage>/
```
