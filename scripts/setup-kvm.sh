#!/usr/bin/env bash
# Ensure /dev/kvm is available in this WSL2 / Linux session.
# WSL2 does not persist loaded kernel modules across restarts, so this may need
# to run once per boot. `make run` also does this check automatically.
set -uo pipefail

usable() { [[ -r /dev/kvm && -w /dev/kvm ]]; }

if usable; then
    echo "/dev/kvm is present and usable."
    exit 0
fi

if [[ -e /dev/kvm ]]; then
    # Present but not openable by this user — the common case on CI runners
    # and on desktops where the user is not in the kvm group.
    echo ">> /dev/kvm exists but is not readable/writable by $(whoami):"
    ls -l /dev/kvm
    echo ">> attempting to grant access..."
    sudo usermod -aG kvm "$(whoami)" 2>/dev/null || true
    sudo chmod 666 /dev/kvm 2>/dev/null || true
    if usable; then
        echo "/dev/kvm is now usable."
        exit 0
    fi
    echo "ERROR: still cannot open /dev/kvm. Add yourself to the kvm group and" >&2
    echo "       start a new shell:  sudo usermod -aG kvm \$USER" >&2
    exit 1
fi

echo ">> /dev/kvm missing; loading KVM modules..."
modprobe kvm         2>/dev/null || sudo modprobe kvm         2>/dev/null || true
modprobe kvm_intel   2>/dev/null || sudo modprobe kvm_intel   2>/dev/null || true
modprobe kvm_amd     2>/dev/null || sudo modprobe kvm_amd     2>/dev/null || true

if [[ -e /dev/kvm ]]; then
    echo "/dev/kvm is now available."
    exit 0
fi

cat >&2 <<'EOF'
ERROR: could not enable /dev/kvm. The hypervisor needs nested virtualization:
  - a Windows 11 host with WSL2 (this project's target), or a Linux host,
  - CPU virtualization (VT-x / AMD-V) enabled in firmware,
  - nested virtualization on (WSL2 enables it by default; check %USERPROFILE%\.wslconfig
    for [wsl2] nestedVirtualization=true if you disabled it).
Confirm the CPU exposes the flag:  grep -o vmx /proc/cpuinfo | head -1
EOF
exit 1
