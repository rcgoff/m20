#!/usr/bin/env bash

readonly M20="$(realpath "$1")"
readonly TEST_DIR="$(realpath "$2")"

if ! [[ -x $M20 ]]; then
  echo "The emulator executable not found: $M20"
  exit 1
fi

if ! [ -d "$TEST_DIR" ]; then
  echo "Test directory not found: $TEST_DIR"
  exit 1
fi

readonly RUN_DIR="$(mktemp -d -t "run-$(date +%Y-%m-%d-%H-%M-%S)-XXXXXXXXXX" --tmpdir="$TEST_DIR")"

echo "RUN TESTS"
echo
echo "M-20 Emulator:     $M20"
echo "Tests Directory:   $TEST_DIR"
echo "Run Directory:     $RUN_DIR"
echo

# Define function to discover existing tests
function list_tests() {
  find . -name "test_*.simh" | sort
}

# Define function to preprocess tests
function preprocess_test() {
  # Change windows 'del' command to unix 'rm' equivalent :(
  sed -ri 's/^! del/! rm -f/' "$1"
}

# Define function to execute single test
function execute_test() {
  local current_test="$1"
  local timeout_interval=10s
  echo -n "$current_test ... "
  preprocess_test "$current_test"
  if timeout --foreground "$timeout_interval" time --output "$current_test.time" --format "%es" --quiet "$M20" "$current_test" 2>"$current_test.output" >&2; then
    local debug_file="${current_test%.simh}_debug.txt"
    if ! grep --quiet "Assertion failed" "$debug_file"; then
      echo "$(tput setaf 2)$(tput bold)SUCCESS$(tput sgr0) ($(cat "$current_test.time"))"
    else
      echo "$(tput setaf 1)$(tput bold)FAILED$(tput sgr0) ($(cat "$current_test.time"))"
      return 1
    fi
  else
    EXIT_CODE=$?
    DETAILS=""
    if [[ "$EXIT_CODE" == 124 ]]; then
      DETAILS=" (timeout after $timeout_interval)"
    else
      DETAILS=" ($(cat "$current_test.time"))"
    fi
    echo "$(tput setaf 1)$(tput bold)ERROR$(tput sgr0)$DETAILS"
    return 1
  fi
}

# Copy tests to the run directory
cp "$TEST_DIR"/*.simh "$TEST_DIR"/*.cdr "$TEST_DIR"/*.m20 "$TEST_DIR"/*.drum0 "$RUN_DIR" || exit 1
cd "$RUN_DIR" || {
  echo "Cannot cd to run directory"
  exit 1
}

# Execute tests
readonly TEST_COUNT="$(list_tests | wc -l)"
echo "Running $TEST_COUNT tests"

FAILED_TESTS=0
EXECUTED_TESTS=0
for test_file in $(list_tests); do
  if ! execute_test "$test_file"; then
    FAILED_TESTS=$((FAILED_TESTS + 1))
  fi
  EXECUTED_TESTS=$((EXECUTED_TESTS + 1))
done

echo
echo "Executed $EXECUTED_TESTS tests, $((EXECUTED_TESTS - FAILED_TESTS)) success, $FAILED_TESTS failed"

if [[ "$FAILED_TESTS" != 0 ]]; then
  exit 1
fi
