#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  scripts/program.sh build --stage STAGE --program PATH [--output PATH]
  scripts/program.sh run --stage STAGE --program PATH [--input PATH] [--output PATH]

Stages:
  L1, L2, L3, IR, LA, LB
EOF
}

build_stage_chain() {
  local stage="$1"

  case "$stage" in
    L1)
      make -C "$ROOT_DIR" --no-print-directory L1_lang
      ;;
    L2)
      make -C "$ROOT_DIR" --no-print-directory L1_lang L2_lang
      ;;
    L3)
      make -C "$ROOT_DIR" --no-print-directory L1_lang L2_lang L3_lang
      ;;
    IR)
      make -C "$ROOT_DIR" --no-print-directory L1_lang L2_lang L3_lang IR_lang
      ;;
    LA)
      make -C "$ROOT_DIR" --no-print-directory L1_lang L2_lang L3_lang IR_lang LA_lang
      ;;
    LB)
      make -C "$ROOT_DIR" --no-print-directory L1_lang L2_lang L3_lang IR_lang LA_lang LB_lang
      ;;
    *)
      echo "Unsupported stage: ${stage}" >&2
      exit 1
      ;;
  esac
}

to_abs_path() {
  local path="$1"
  if [[ "$path" = /* ]]; then
    printf '%s\n' "$path"
  else
    printf '%s/%s\n' "$ROOT_DIR" "$path"
  fi
}

default_output_path() {
  local stage_lower="$1"
  local program_path="$2"
  local program_name
  program_name="$(basename "$program_path")"
  program_name="${program_name%.*}"
  printf '%s/build/programs/%s/%s.out\n' "$ROOT_DIR" "$stage_lower" "$program_name"
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

ACTION="$1"
shift

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

STAGE=""
PROGRAM=""
INPUT=""
OUTPUT=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --stage)
      STAGE="${2:-}"
      shift 2
      ;;
    --program)
      PROGRAM="${2:-}"
      shift 2
      ;;
    --input)
      INPUT="${2:-}"
      shift 2
      ;;
    --output)
      OUTPUT="${2:-}"
      shift 2
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

case "$STAGE" in
  L1|L2|L3|IR|LA|LB)
    ;;
  *)
    echo "Unsupported or missing stage: ${STAGE}" >&2
    usage
    exit 1
    ;;
esac

if [[ -z "$PROGRAM" ]]; then
  echo "--program is required." >&2
  usage
  exit 1
fi

PROGRAM_ABS="$(to_abs_path "$PROGRAM")"
if [[ ! -f "$PROGRAM_ABS" ]]; then
  echo "Program not found: $PROGRAM_ABS" >&2
  exit 1
fi

STAGE_LOWER="$(printf '%s' "$STAGE" | tr '[:upper:]' '[:lower:]')"
OUTPUT_ABS="$(to_abs_path "${OUTPUT:-$(default_output_path "$STAGE_LOWER" "$PROGRAM_ABS")}")"
mkdir -p "$(dirname "$OUTPUT_ABS")"

if [[ -n "$INPUT" ]]; then
  INPUT_ABS="$(to_abs_path "$INPUT")"
  if [[ ! -f "$INPUT_ABS" ]]; then
    echo "Input file not found: $INPUT_ABS" >&2
    exit 1
  fi
else
  INPUT_ABS=""
fi

STAGE_DIR="${ROOT_DIR}/src/${STAGE}"
COMPILER_WRAPPER="${STAGE_DIR}/${STAGE}c"
if [[ ! -x "$COMPILER_WRAPPER" ]]; then
  echo "Compiler wrapper not found or not executable: $COMPILER_WRAPPER" >&2
  exit 1
fi

build_stage_chain "$STAGE"

(
  cd "$STAGE_DIR"
  rm -f a.out
  "./${STAGE}c" "$PROGRAM_ABS"
)

if [[ ! -f "${STAGE_DIR}/a.out" ]]; then
  echo "Compilation did not produce an executable." >&2
  exit 1
fi

mv "${STAGE_DIR}/a.out" "$OUTPUT_ABS"
echo "Built ${OUTPUT_ABS}"

if [[ "$ACTION" = "run" ]]; then
  if [[ -n "$INPUT_ABS" ]]; then
    "$OUTPUT_ABS" < "$INPUT_ABS"
  else
    "$OUTPUT_ABS"
  fi
elif [[ "$ACTION" != "build" ]]; then
  echo "Unsupported action: ${ACTION}" >&2
  usage
  exit 1
fi
