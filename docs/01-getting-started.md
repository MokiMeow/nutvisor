# 01 — Getting started

Everything here is free and local. On Windows, do all of this **inside WSL2**
(Ubuntu). On Linux, run it directly.

## 1. Requirements: nested virtualization + /dev/kvm

nutvisor runs guests on real hardware virtualization, so it needs `/dev/kvm`.
On Windows 11 + WSL2 this works because WSL2 exposes the CPU's VT-x to the Linux
guest (nested virtualization is on by default).

Confirm the CPU flag is visible:

```bash
grep -o vmx /proc/cpuinfo | head -1     # prints "vmx" on Intel (svm on AMD)
```

Then ensure the device exists:

```bash
./scripts/setup-kvm.sh
```

WSL2 does **not** persist loaded kernel modules across a restart, so `/dev/kvm`
may disappear after `wsl --shutdown`. That's fine — `make run` reloads the KVM
module automatically, and you can always re-run the setup script.

If `/dev/kvm` still can't be created, check:
- `.wslconfig` in your Windows user folder has `[wsl2] nestedVirtualization=true`
  (this is the default; only an issue if you turned it off),
- CPU virtualization (VT-x/AMD-V) is enabled in your firmware/BIOS.

## 2. Install the toolchain (one time)

```bash
sudo apt-get update && sudo apt-get install -y gcc make nasm
```

The KVM headers (`<linux/kvm.h>`) ship with the base system; there is nothing
else to install.

## 3. Build and run

```bash
make run
```

This builds `build/nutvisor` and all guest images, ensures
`/dev/kvm`, and runs the guest. Expected output:

```
nutvisor: running build/mmio-demo.bin (...) in 64-bit long mode
nutvisor: mmio console online
nutvisor: guest requested exit status=0
nutvisor: guest exited cleanly
```

The MMIO marker is printed *by the 64-bit guest* through an emulated memory-
mapped console; the same guest then stops through the MMIO exit register.

## 4. Running a different guest

`nutvisor` takes a guest image path:

```bash
make all
./build/nutvisor --long build/hello64.bin
./build/nutvisor --real build/hello16.bin
```

An explicit image without a mode flag retains the original real-mode interface.

## 5. Make targets

| Command | Purpose |
|---------|---------|
| `make all` | Build the VMM and guest image(s). |
| `make run` | Ensure `/dev/kvm`, build, and run the MMIO guest. |
| `make run-long` | Run the milestone-2 long-mode serial guest. |
| `make run-serial` | Run the milestone-1 UART-driver guest. |
| `make run-hello16` | Run the original milestone-0 guest. |
| `make check-kvm` | Just ensure `/dev/kvm` is available. |
| `make clean` | Delete `build/`. |

## Troubleshooting

- **`open /dev/kvm: No such file or directory`** — run `./scripts/setup-kvm.sh`;
  if it fails, nested virtualization isn't available (see §1).
- **`open /dev/kvm: Permission denied`** — your user isn't in the `kvm` group.
  On this project's WSL setup the default user is root, so this shouldn't occur;
  otherwise `sudo usermod -aG kvm $USER` and restart the shell.
- **`KVM_EXIT_SHUTDOWN` / guest shutdown** — the guest triple-faulted. See
  [docs/09](09-testing-and-debugging.md).
- **`unhandled exit_reason=N`** — the guest did something the current milestone
  doesn't emulate yet; that's the next milestone's job.
