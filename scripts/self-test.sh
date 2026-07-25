#!/usr/bin/env bash
# End-to-end regression suite. Every assertion is based on output from a guest
# that actually ran through KVM; failure-path cases must return nonzero.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_dir"

selftest_dir=$(mktemp -d)
trap 'rm -rf -- "$selftest_dir"' EXIT

timeout_seconds=${SELFTEST_TIMEOUT:-60}
last_log=

run_success() {
    local name=$1
    shift
    last_log="$selftest_dir/$name.log"
    if ! timeout --foreground "$timeout_seconds" "$@" >"$last_log" 2>&1; then
        echo "[FAIL] $name: expected success" >&2
        cat "$last_log" >&2
        exit 1
    fi
}

run_failure() {
    local name=$1
    shift
    last_log="$selftest_dir/$name.log"
    if timeout --foreground "$timeout_seconds" "$@" >"$last_log" 2>&1; then
        echo "[FAIL] $name: expected failure" >&2
        cat "$last_log" >&2
        exit 1
    fi
}

require_marker() {
    local marker=$1
    if ! grep -Fq -- "$marker" "$last_log"; then
        echo "[FAIL] missing marker: $marker" >&2
        cat "$last_log" >&2
        exit 1
    fi
}

vmm=./build/nutvisor

run_success elf-kernel "$vmm" guests/kernel/kernel.elf
require_marker "nutvisor: cpuid online"
require_marker "nutvisor: elf64 kernel online"
require_marker "nutvisor: guest halted cleanly"
echo "[ok] elf-kernel"

run_success mmio "$vmm" --long build/mmio-demo.bin
require_marker "nutvisor: mmio console online"
require_marker "nutvisor: guest requested exit status=0"
require_marker "nutvisor: guest exited cleanly"
echo "[ok] mmio"

run_success long-mode "$vmm" --long build/hello64.bin
require_marker "nutvisor: long mode online"
echo "[ok] long-mode"

run_success serial-driver "$vmm" --real build/serial-driver.bin
require_marker "nutvisor: 16550 driver online"
echo "[ok] serial-driver"

run_success real-mode "$vmm" --real build/hello16.bin
require_marker "nutvisor: the guest is alive inside your hypervisor"
echo "[ok] real-mode"

run_failure malformed-elf "$vmm" --elf README.md
require_marker "ELF: invalid magic"
echo "[ok] malformed-elf"

run_failure short-elf "$vmm" --elf build/fault64.bin
require_marker "ELF: file is smaller than the ELF64 header"
echo "[ok] short-elf"

run_failure triple-fault "$vmm" --long build/fault64.bin
require_marker "guest shutdown (triple fault?)"
require_marker "registers: rip="
require_marker "control: cr0="
echo "[ok] triple-fault-diagnostics"

echo "[ok] selftest"
