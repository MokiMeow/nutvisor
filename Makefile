# nutvisor — build system
#
# Quick start (WSL2 / Linux with nested virtualization):
#   ./scripts/setup-kvm.sh    # ensure /dev/kvm is available (one time per boot)
#   make run                  # build the VMM + guests and run the current guest

CC     := gcc
NASM   := nasm
# gnu11 (not c11) so POSIX/Linux symbols like O_CLOEXEC and MAP_ANONYMOUS from
# <fcntl.h>/<sys/mman.h> are visible without hand-rolling feature-test macros.
CFLAGS := -std=gnu11 -Wall -Wextra -Iinclude -O2
BUILD  := build

VMM_SRC := $(wildcard src/*.c)
VMM_OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(VMM_SRC))
VMM     := $(BUILD)/nutvisor

GUEST_SRC := guests/hello16.asm
GUEST_BIN := $(BUILD)/hello16.bin
SERIAL_GUEST_SRC := guests/serial-driver.asm
SERIAL_GUEST_BIN := $(BUILD)/serial-driver.bin

.PHONY: all run run-hello16 check-kvm clean

all: $(VMM) $(GUEST_BIN) $(SERIAL_GUEST_BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(VMM): $(VMM_OBJ)
	$(CC) $(CFLAGS) -o $@ $(VMM_OBJ)

$(GUEST_BIN): $(GUEST_SRC) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(SERIAL_GUEST_BIN): $(SERIAL_GUEST_SRC) | $(BUILD)
	$(NASM) -f bin $< -o $@

# Load the KVM module if the device node is missing (needed after a WSL
# restart). Note the device *existing* says nothing about whether this user can
# open it, so check read/write access too — that distinction is what made the
# guest fail on CI runners, which ship a root-only /dev/kvm.
check-kvm:
	@if [ ! -e /dev/kvm ]; then \
	  echo ">> /dev/kvm missing; loading kvm_intel..."; \
	  modprobe kvm_intel 2>/dev/null || sudo modprobe kvm_intel 2>/dev/null || true; \
	fi
	@test -e /dev/kvm || { \
	  echo "ERROR: /dev/kvm does not exist. Run ./scripts/setup-kvm.sh"; \
	  echo "       (see docs/01-getting-started.md for nested-virt requirements)."; \
	  exit 1; }
	@{ test -r /dev/kvm && test -w /dev/kvm; } || { \
	  echo "ERROR: /dev/kvm exists but this user cannot open it."; \
	  echo "       Fix: sudo usermod -aG kvm $$USER   (then start a new shell),"; \
	  echo "       or run as root. Current permissions:"; \
	  ls -l /dev/kvm; \
	  exit 1; }

run: check-kvm $(VMM) $(SERIAL_GUEST_BIN)
	$(VMM) $(SERIAL_GUEST_BIN)

run-hello16: check-kvm $(VMM) $(GUEST_BIN)
	$(VMM) $(GUEST_BIN)

clean:
	rm -rf $(BUILD)
