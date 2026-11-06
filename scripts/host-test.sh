#!/usr/bin/env bash
# CLI/preflight regression tests: no KVM device is needed or opened.
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_dir"
vmm=${VMM:-./build/nutvisor}
build_dir=${BUILD_DIR:-build}
test_dir=$(mktemp -d)
trap 'rm -rf -- "$test_dir"' EXIT

expect() {
    local expected=$1 marker=$2 status=0
    shift 2
    "$@" >"$test_dir/output" 2>&1 || status=$?
    if [[ "$status" != "$expected" ]] || ! grep -Fq -- "$marker" "$test_dir/output"; then
        printf '[FAIL] expected status=%s marker=%s; got status=%s\n' \
            "$expected" "$marker" "$status" >&2
        cat "$test_dir/output" >&2
        exit 1
    fi
}

expect 0 'usage:' "$vmm" --help
expect 2 'usage:' "$vmm" --unknown
expect 2 'usage:' "$vmm" --long
expect 2 'usage:' "$vmm" --real --elf README.md
expect 2 'usage:' "$vmm" --check --check README.md
expect 0 'image valid, entry=0x100000' "$vmm" --check guests/kernel/kernel.elf
expect 0 'image valid, entry=0x100000' "$vmm" --elf --check guests/kernel/kernel.elf
expect 0 'image valid, entry=0x1000' "$vmm" --check --real "$build_dir/hello16.bin"
expect 0 'image valid, entry=0x100000' "$vmm" --check --long "$build_dir/hello64.bin"
expect 1 'ELF: invalid magic' "$vmm" --check --elf README.md
# Invalid input must be rejected before /dev/kvm even in execution mode.
expect 1 'ELF: invalid magic' "$vmm" --elf README.md
expect 1 'ELF: file is smaller' "$vmm" --check --elf "$build_dir/fault64.bin"
: >"$test_dir/empty"
expect 1 'guest image is empty' "$vmm" --check "$test_dir/empty"
expect 1 'No such file' "$vmm" --check "$test_dir/missing"
# Sparse files prove the allocation cap without consuming 128 MiB of disk/RAM.
truncate -s 134217729 "$test_dir/oversized"
expect 1 '128 MiB file limit' "$vmm" --check --elf "$test_dir/oversized"
truncate -s $((0x7f0000 - 0x100000)) "$test_dir/flat"
expect 0 'image valid' "$vmm" --check --long "$test_dir/flat"
truncate -s $((0x7f0000 - 0x100000 + 1)) "$test_dir/flat"
expect 1 'overlaps bootstrap memory' "$vmm" --check --long "$test_dir/flat"
# Real-mode blobs do not use the long-mode bootstrap tables/stack.
expect 0 'image valid' "$vmm" --check --real "$test_dir/flat"
echo '[ok] host CLI: 19 preflight, limit and option checks'
