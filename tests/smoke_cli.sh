#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-/home/pavan/Programming/mgit/build/linux/x86_64/debug/mgit}"
if command -v realpath >/dev/null 2>&1; then
  BIN="$(realpath "$BIN")"
fi
if [[ ! -x "$BIN" ]]; then
  echo "Binary not found or not executable: $BIN" >&2
  echo "Pass binary path as first arg, e.g. ./tests/smoke_cli.sh ./build/linux/x86_64/debug/mgit" >&2
  exit 1
fi

TESTROOT=$(mktemp -d /tmp/mgit-cmdtest-XXXXXX)
REMOTE_ROOT=$(mktemp -d /tmp/mgit-remote-XXXXXX)
ARTIFACTS=$(mktemp -d /tmp/mgit-artifacts-XXXXXX)
LOG="$ARTIFACTS/results.log"
SUMMARY="$ARTIFACTS/summary.tsv"
: > "$LOG"
: > "$SUMMARY"

run_expect() {
  local name="$1"; shift
  local expect="$1"; shift
  echo -e "\n### $name" >> "$LOG"
  echo "CMD: $*" >> "$LOG"
  set +e
  "$@" >> "$LOG" 2>&1
  local rc=$?
  set -e
  local ok=0
  [[ "$expect" == "0" && $rc -eq 0 ]] && ok=1
  [[ "$expect" == "nonzero" && $rc -ne 0 ]] && ok=1
  printf "%s\t%s\t%s\n" "$name" "$rc" "$ok" >> "$SUMMARY"
}

cd "$TESTROOT"
run_expect "init" 0 bash -lc "printf 'Test User\ntest@example.com\n' | '$BIN' init .git"
printf 'hello\n' > a.txt
run_expect "hash-object" 0 "$BIN" hash-object -w a.txt
run_expect "add" 0 "$BIN" add a.txt
run_expect "write-tree" 0 "$BIN" write-tree
TREE_HASH=$(awk '/Tree object written:/ {print $4}' "$LOG" | tail -n1)
if [[ -z "${TREE_HASH}" && -f .git/index ]]; then
  TREE_HASH=$(awk -F'\t' 'NR==1{print $3}' .git/index || true)
fi
run_expect "commit" 0 "$BIN" commit -m "initial commit"
HEAD_HASH=""
[[ -f .git/refs/heads/main ]] && HEAD_HASH=$(cat .git/refs/heads/main)

run_expect "cat-file-type-head" 0 "$BIN" cat-file -t HEAD
run_expect "cat-file-pretty-head" 0 "$BIN" cat-file -p HEAD
run_expect "read-object-head" 0 "$BIN" read-object "$HEAD_HASH"
run_expect "ls-read-head" 0 "$BIN" ls-read "$HEAD_HASH"
run_expect "ls-tree" 0 "$BIN" ls-tree "$TREE_HASH"
run_expect "ls-tree-r" 0 "$BIN" ls-tree-r "$TREE_HASH"

run_expect "commit-tree" 0 "$BIN" commit-tree "$TREE_HASH" -p "$HEAD_HASH" -m "plumbing commit"
PLUMB_HASH=$(awk '/Commit object written:/ {print $4}' "$LOG" | tail -n1)
run_expect "tag" 0 "$BIN" tag "$PLUMB_HASH" commit v1 -m "tag message"

run_expect "status" 0 "$BIN" status
run_expect "branch-create" 0 "$BIN" branch feature
run_expect "branch-list" 0 "$BIN" branch -l
run_expect "branch-current" 0 "$BIN" branch --show-current
run_expect "switch-feature" 0 "$BIN" switch feature
run_expect "checkout-main" 0 "$BIN" checkout main

run_expect "merge-feature" 0 "$BIN" merge feature
run_expect "merge-status" 0 "$BIN" merge-status
run_expect "merge-continue" nonzero "$BIN" merge --continue
run_expect "merge-abort" 0 "$BIN" merge --abort

run_expect "config-set" 0 "$BIN" config user.name "Test User 2"
run_expect "config-get" 0 "$BIN" config user.name

run_expect "remote-init" 0 bash -lc "cd '$REMOTE_ROOT' && printf 'Remote User\nremote@example.com\n' | '$BIN' init .git"
run_expect "remote-add" 0 "$BIN" remote add origin "$REMOTE_ROOT/.git"
run_expect "remote-list" 0 "$BIN" remote list
run_expect "push" 0 "$BIN" push origin
bash -lc "cd '$REMOTE_ROOT' && printf 'remote file\n' > r.txt && '$BIN' add r.txt && '$BIN' commit -m 'remote commit'" >> "$LOG" 2>&1 || true
run_expect "pull" 0 "$BIN" pull origin
run_expect "remote-remove" 0 "$BIN" remote remove origin

run_expect "activity-summary" 0 "$BIN" activity summary
run_expect "resolve-conflict-noop" nonzero "$BIN" resolve-conflict a.txt deadbeef
run_expect "log-placeholder" nonzero "$BIN" log

TOTAL=$(wc -l < "$SUMMARY" | tr -d ' ')
PASS=$(awk -F'\t' '$3==1{c++} END{print c+0}' "$SUMMARY")
FAIL=$((TOTAL-PASS))

echo "TESTROOT=$TESTROOT"
echo "REMOTE_ROOT=$REMOTE_ROOT"
echo "ARTIFACTS=$ARTIFACTS"
echo "TOTAL=$TOTAL PASS=$PASS FAIL=$FAIL"
echo "---FAILURES---"
awk -F'\t' '$3!=1{print $1" rc="$2}' "$SUMMARY" || true
echo "---SUMMARY FILE---"
echo "$SUMMARY"
echo "---LOG FILE---"
echo "$LOG"
