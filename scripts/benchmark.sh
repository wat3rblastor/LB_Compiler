#!/usr/bin/env bash

set -u
set -o pipefail

usage() {
  cat <<'EOF'
Usage: benchmark.sh [options]

Benchmark LB source tests from the repo-level `tests/` directory by compiling
with `src/LB/LBc` and timing `./a.out`.
Reported timing metrics are in milliseconds.

Options:
  -r RUNS           Number of measured runs per benchmark (default: 7)
  -w WARMUPS        Number of warmup runs per benchmark (default: 2)
  -t TESTS_DIR      Tests directory relative to the repo root (default: tests)
  -p PATTERN        File glob for benchmark files (default: *.b)
  -l LIST_FILE      File containing benchmark paths (one per line, relative to the repo root)
  -o OUTPUT_CSV     Output CSV path (default: /tmp/lb_benchmark_results.csv)
  -c COMPILER       Compiler command relative to src/LB/ (default: ./LBc)
  --no-check        Do not compare runtime output with .out oracle
  -h, --help        Show this help

Examples:
  scripts/benchmark.sh
  scripts/benchmark.sh -r 15 -w 2
  scripts/benchmark.sh -l /tmp/benchmarks.txt -o /tmp/results.csv
EOF
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
LB_DIR="${ROOT_DIR}/src/LB"
LB_TMP_DIR="${ROOT_DIR}/build/LB/tmp"
EXECUTABLE_PATH="${LB_TMP_DIR}/a.out"

RUNS=7
WARMUPS=2
TESTS_DIR="tests"
PATTERN="*.b"
LIST_FILE=""
OUTPUT_CSV="/tmp/lb_benchmark_results.csv"
COMPILER="./LBc"
CHECK_OUTPUT=1
REQUIRE_ORACLE=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    -r)
      RUNS="${2:-}"
      shift 2
      ;;
    -w)
      WARMUPS="${2:-}"
      shift 2
      ;;
    -t)
      TESTS_DIR="${2:-}"
      shift 2
      ;;
    -p)
      PATTERN="${2:-}"
      shift 2
      ;;
    -l)
      LIST_FILE="${2:-}"
      shift 2
      ;;
    -o)
      OUTPUT_CSV="${2:-}"
      shift 2
      ;;
    -c)
      COMPILER="${2:-}"
      shift 2
      ;;
    --no-check)
      CHECK_OUTPUT=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

is_nonneg_int() {
  [[ "$1" =~ ^[0-9]+$ ]]
}

if ! is_nonneg_int "$RUNS" || [[ "$RUNS" -eq 0 ]]; then
  echo "RUNS must be a positive integer." >&2
  exit 1
fi

if ! is_nonneg_int "$WARMUPS"; then
  echo "WARMUPS must be a non-negative integer." >&2
  exit 1
fi

if [[ ! -d "$LB_DIR" ]]; then
  echo "LB directory not found: $LB_DIR" >&2
  exit 1
fi

collect_benchmarks() {
  local -n out_ref=$1
  out_ref=()

  if [[ -n "$LIST_FILE" ]]; then
    if [[ ! -f "$LIST_FILE" ]]; then
      echo "Benchmark list file not found: $LIST_FILE" >&2
      return 1
    fi
    while IFS= read -r line; do
      line="${line%%#*}"
      line="$(echo "$line" | xargs)"
      [[ -z "$line" ]] && continue
      out_ref+=("$line")
    done < "$LIST_FILE"
  else
    local test_abs="${ROOT_DIR}/${TESTS_DIR}"
    if [[ ! -d "$test_abs" ]]; then
      echo "Tests directory not found: $test_abs" >&2
      return 1
    fi

    while IFS= read -r file; do
      out_ref+=("${file#${ROOT_DIR}/}")
    done < <(find "$test_abs" -type f -name "$PATTERN" | sort)
  fi

  if [[ "${#out_ref[@]}" -eq 0 ]]; then
    echo "No benchmarks found." >&2
    return 1
  fi
}

calc_stats() {
  local values="$1"
  local sorted
  sorted="$(printf "%s\n" "$values" | awk 'NF{print $1}' | sort -n)"
  awk '
    {
      a[NR]=$1;
      sum+=$1;
      if (NR==1 || $1<min) min=$1;
      if (NR==1 || $1>max) max=$1;
    }
    END {
      if (NR==0) {
        print "0,0,0,0";
        exit;
      }
      n=NR;
      if (n%2==1) {
        median=a[(n+1)/2];
      } else {
        median=(a[n/2]+a[n/2+1])/2.0;
      }
      avg=sum/n;
      # Input values are seconds; convert reported stats to milliseconds.
      printf "%.3f,%.3f,%.3f,%.3f\n", avg*1000, median*1000, min*1000, max*1000;
    }
  ' <<< "$sorted"
}

run_once_timed() {
  local input_rel="$1"
  local t

  if [[ -n "$input_rel" ]]; then
    t=$(
      cd "$ROOT_DIR" && \
      TIMEFORMAT='%R' && \
      { time ("$EXECUTABLE_PATH" < "$input_rel" > /dev/null 2>&1); } 2>&1
    )
  else
    t=$(
      cd "$ROOT_DIR" && \
      TIMEFORMAT='%R' && \
      { time ("$EXECUTABLE_PATH" > /dev/null 2>&1); } 2>&1
    )
  fi
  echo "$t"
}

run_once_untimed() {
  local input_rel="$1"
  if [[ -n "$input_rel" ]]; then
    (cd "$ROOT_DIR" && "$EXECUTABLE_PATH" < "$input_rel" > /dev/null 2>&1)
  else
    (cd "$ROOT_DIR" && "$EXECUTABLE_PATH" > /dev/null 2>&1)
  fi
}

run_and_capture_output() {
  local input_rel="$1"
  local output_file="$2"
  if [[ -n "$input_rel" ]]; then
    (cd "$ROOT_DIR" && "$EXECUTABLE_PATH" < "$input_rel" > "$output_file" 2>&1)
  else
    (cd "$ROOT_DIR" && "$EXECUTABLE_PATH" > "$output_file" 2>&1)
  fi
}

benchmarks=()
collect_benchmarks benchmarks || exit 1

mkdir -p "$LB_TMP_DIR"
mkdir -p "$(dirname "$OUTPUT_CSV")"
echo "benchmark,status,runs,warmups,avg_ms,median_ms,min_ms,max_ms" > "$OUTPUT_CSV"

pass_count=0
fail_count=0

echo "Running ${#benchmarks[@]} LB benchmarks..."
echo "Results CSV: $OUTPUT_CSV"

for bench_rel in "${benchmarks[@]}"; do
  bench_abs="${ROOT_DIR}/${bench_rel}"
  if [[ ! -f "$bench_abs" ]]; then
    echo "SKIP (missing): $bench_rel"
    echo "$bench_rel,missing,0,0,,,," >> "$OUTPUT_CSV"
    fail_count=$((fail_count + 1))
    continue
  fi

  input_rel=""
  output_oracle_rel=""
  if [[ -f "${bench_abs}.in" ]]; then
    input_rel="${bench_rel}.in"
  fi
  if [[ -f "${bench_abs}.out" ]]; then
    output_oracle_rel="${bench_rel}.out"
  fi

  if [[ "$REQUIRE_ORACLE" -eq 1 && -z "$output_oracle_rel" ]]; then
    echo "  FAIL: missing oracle output (${bench_rel}.out)"
    echo "$bench_rel,missing_oracle,0,0,,,," >> "$OUTPUT_CSV"
    fail_count=$((fail_count + 1))
    continue
  fi

  echo "Benchmark: $bench_rel"

  rm -f "$EXECUTABLE_PATH" "${LB_DIR}/a.out"
  if ! (cd "$LB_DIR" && $COMPILER "../../${bench_rel}" > /dev/null 2>&1); then
    echo "  FAIL: compile"
    echo "$bench_rel,compile_fail,0,0,,,," >> "$OUTPUT_CSV"
    fail_count=$((fail_count + 1))
    continue
  fi

  if [[ ! -f "${LB_DIR}/a.out" ]]; then
    echo "  FAIL: compile produced no a.out"
    echo "$bench_rel,no_binary,0,0,,,," >> "$OUTPUT_CSV"
    fail_count=$((fail_count + 1))
    continue
  fi

  mv "${LB_DIR}/a.out" "$EXECUTABLE_PATH"

  if [[ "$CHECK_OUTPUT" -eq 1 && -n "$output_oracle_rel" ]]; then
    tmp_out="$(mktemp)"
    if ! run_and_capture_output "$input_rel" "$tmp_out"; then
      echo "  FAIL: runtime during oracle check"
      rm -f "$tmp_out"
      echo "$bench_rel,runtime_fail,0,0,,,," >> "$OUTPUT_CSV"
      fail_count=$((fail_count + 1))
      continue
    fi
    if ! cmp -s "$tmp_out" "${ROOT_DIR}/${output_oracle_rel}"; then
      echo "  FAIL: output mismatch vs ${output_oracle_rel}"
      rm -f "$tmp_out"
      echo "$bench_rel,output_mismatch,0,0,,,," >> "$OUTPUT_CSV"
      fail_count=$((fail_count + 1))
      continue
    fi
    rm -f "$tmp_out"
  fi

  warmup_ok=1
  for ((i=0; i<WARMUPS; i++)); do
    if ! run_once_untimed "$input_rel"; then
      warmup_ok=0
      break
    fi
  done
  if [[ "$warmup_ok" -ne 1 ]]; then
    echo "  FAIL: runtime during warmup"
    echo "$bench_rel,warmup_runtime_fail,0,$WARMUPS,,,," >> "$OUTPUT_CSV"
    fail_count=$((fail_count + 1))
    continue
  fi

  times_buf=""
  measured_ok=1
  for ((i=0; i<RUNS; i++)); do
    t="$(run_once_timed "$input_rel")"
    if [[ ! "$t" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
      measured_ok=0
      break
    fi
    times_buf+="${t}"$'\n'
  done

  if [[ "$measured_ok" -ne 1 ]]; then
    echo "  FAIL: runtime during measured runs"
    echo "$bench_rel,measured_runtime_fail,0,$WARMUPS,,,," >> "$OUTPUT_CSV"
    fail_count=$((fail_count + 1))
    continue
  fi

  stats="$(calc_stats "$times_buf")"
  avg="$(echo "$stats" | cut -d',' -f1)"
  median="$(echo "$stats" | cut -d',' -f2)"
  minv="$(echo "$stats" | cut -d',' -f3)"
  maxv="$(echo "$stats" | cut -d',' -f4)"

  if [[ "$RUNS" -eq 1 ]]; then
    echo "  OK: runtime=${avg}ms"
  else
    echo "  OK: avg=${avg}ms median=${median}ms min=${minv}ms max=${maxv}ms"
  fi
  echo "$bench_rel,ok,$RUNS,$WARMUPS,$avg,$median,$minv,$maxv" >> "$OUTPUT_CSV"
  pass_count=$((pass_count + 1))
done

echo
echo "Summary: pass=$pass_count fail=$fail_count total=${#benchmarks[@]}"
echo "CSV written to: $OUTPUT_CSV"

exit 0
