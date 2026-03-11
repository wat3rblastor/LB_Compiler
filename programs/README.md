Place user-authored programs under `programs/<stage>/scratch/`.

Suggested stage directories:
- `programs/l1/scratch/`
- `programs/l2/scratch/`
- `programs/l3/scratch/`
- `programs/ir/scratch/`
- `programs/la/scratch/`
- `programs/lb/scratch/`

Build from the repo root with:
- `make exec_l1 PROGRAM=programs/l1/scratch/example.L1`
- `make exec_l2 PROGRAM=programs/l2/scratch/example.L2`
- `make exec_l3 PROGRAM=programs/l3/scratch/example.L3`
- `make exec_ir PROGRAM=programs/ir/scratch/example.IR`
- `make exec_la PROGRAM=programs/la/scratch/example.a`
- `make exec_lb PROGRAM=programs/lb/scratch/example.b`

Run from the repo root with:
- `make run_lb PROGRAM=programs/lb/scratch/example.b`
- `make run_lb PROGRAM=programs/lb/scratch/example.b INPUT=programs/lb/scratch/example.in`

The repository includes a minimal LB example in `programs/lb/scratch/example.b`.

Generated executables are written to `build/programs/<stage>/` by default.
