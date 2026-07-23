# nutvisor — build system
#
# Quick start (WSL2 / Linux with nested virtualization):
#   ./scripts/setup-kvm.sh    # ensure /dev/kvm is available (one time per boot)
#   make run                  # build the VMM + guest and run milestone 0

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

.PHONY: all run check-kvm clean

all: $(VMM) $(GUEST_BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(VMM): $(VMM_OBJ)
	$(CC) $(CFLAGS) -o $@ $(VMM_OBJ)

$(GUEST_BIN): $(GUEST_SRC) | $(BUILD)
	$(NASM) -f bin $< -o $@

# Load the KVM module if the device node is missing (needed after a WSL restart).
check-kvm:
	@if [ ! -e /dev/kvm ]; then \
	  echo ">> /dev/kvm missing; loading kvm_intel..."; \
	  modprobe kvm_intel 2>/dev/null || sudo modprobe kvm_intel 2>/dev/null || true; \
	fi
	@test -e /dev/kvm || { echo "ERROR: /dev/kvm unavailable. Run ./scripts/setup-kvm.sh (see docs/01-getting-started.md)."; exit 1; }

run: check-kvm $(VMM) $(GUEST_BIN)
	$(VMM) $(GUEST_BIN)

clean:
	rm -rf $(BUILD)
