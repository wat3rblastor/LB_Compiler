# LB Compiler

This repository contains a multi-stage compiler pipeline that lowers programs from `LB` down to `x86-64`.

```text
LB -> LA -> IR -> L3 -> L2 -> L1 -> x86-64
```

The compiler wrappers are:

- `LBc`
- `LAc`
- `IRc`
- `L3c`
- `L2c`
- `L1c`

Each stage has a focused role:

- `LBc` removes higher-level structure such as nested scopes and structured control flow.
- `LAc` encodes values and inserts access checks.
- `IRc` applies optimization passes and lowers array-related structure.
- `L3c` performs instruction selection.
- `L2c` performs register allocation.
- `L1c` emits `x86-64` assembly, which is then assembled and linked into an executable.

## Layout

```text
.
├── build/      # Generated compiler binaries, objects, and user-program executables
├── docker/     # Dockerfile and docker-compose.yml
├── lib/        # Shared libraries and runtime support
├── programs/   # User-authored scratch programs
├── scripts/    # Shared helper scripts
├── src/        # Compiler stages
├── tests/      # Official LB tests and benchmark inputs
└── Makefile
```

Generated stage artifacts are written to:

```text
build/<stage>/bin/
build/<stage>/obj/
```

Generated user-program executables are written to:

```text
build/programs/<stage>/
```

## Requirements

The recommended path is Docker, since it gives a reproducible build and run environment.

If you want to build directly on a Linux-like environment, you need:

- `g++` with C++17 support
- `make`
- `bash`
- `ncurses-bin` or equivalent `tput`
- `time`
- `valgrind`

Or use Docker.

## Docker

From the repo root:

```sh
docker compose -f docker/docker-compose.yml up -d --build
docker compose -f docker/docker-compose.yml exec dev bash
```

Inside the container, the main workflow is:

```sh
make
make test
make benchmark
```

## Build

Build all stages:

```sh
make
```

Build a single stage:

```sh
make L1_lang
make L2_lang
make L3_lang
make IR_lang
make LA_lang
make LB_lang
```

## Test

Run the official LB test corpus:

```sh
make test
```

The official LB test programs live in `tests/`.

## Benchmark

Run the LB benchmark suite:

```sh
make benchmark
```

Benchmark results are written to:

```text
benchmark_results.csv
```

The benchmark harness checks program output against the expected oracle before recording timing results.
The default benchmark target runs `7` measured executions with `2` warmup runs per program.

## User Programs

Put your own programs under:

```text
programs/<stage>/scratch/
```

For example:

- `programs/l1/scratch/`
- `programs/l2/scratch/`
- `programs/l3/scratch/`
- `programs/ir/scratch/`
- `programs/la/scratch/`
- `programs/lb/scratch/`

Build an executable from the repo root:

```sh
make exec_lb PROGRAM=programs/lb/scratch/example.b
```

Run a user program from the repo root:

```sh
make run_lb PROGRAM=programs/lb/scratch/example.b
make run_lb PROGRAM=programs/lb/scratch/example.b INPUT=programs/lb/scratch/example.in
```

The repository includes a minimal LB sample program at `programs/lb/scratch/example.b`.

Equivalent targets exist for every stage:

- `exec_l1`, `exec_l2`, `exec_l3`, `exec_ir`, `exec_la`, `exec_lb`
- `run_l1`, `run_l2`, `run_l3`, `run_ir`, `run_la`, `run_lb`

You can override the default executable output path with `OUTPUT=...`.

## Clean

Remove generated build artifacts:

```sh
make clean
```

## More Detail

For a short description of the pipeline design and the intended reproducible workflow, see `DESIGN.md`.
