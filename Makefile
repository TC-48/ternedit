CC ?= cc
BUILD ?= release

ifeq ($(OS),Windows_NT)
	PLATFORM := windows
else
	PLATFORM := posix
endif

SRC_DIR     := src
INCLUDE_DIR := include

OBJ_ROOT_DIR := build/$(BUILD)/obj
DEP_ROOT_DIR := build/$(BUILD)/dep

OUT_DIR     := out/$(BUILD)
BIN_DIR     := $(OUT_DIR)/bin

EXE_EXT :=
ifeq ($(PLATFORM),windows)
	EXE_EXT := .exe
else ifeq ($(PLATFORM),posix)
	EXE_EXT := .elf
endif
TARGET := $(BIN_DIR)/ternedit$(EXE_EXT)

CSTD     := -std=c11
WARNINGS := -Wall -Wextra

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf)
SDL_LIBS   := $(shell pkg-config --libs sdl2 SDL2_ttf)

COMMON_CFLAGS := $(CSTD) $(WARNINGS) -I$(INCLUDE_DIR) $(SDL_CFLAGS)

ifeq ($(BUILD),debug)
	CFLAGS := $(COMMON_CFLAGS) -O0 -g -DEL_DEBUG -fsanitize=address,undefined
	LDFLAGS := $(SDL_LIBS) -fsanitize=address,undefined
else ifeq ($(BUILD),release)
	CFLAGS := $(COMMON_CFLAGS) -O3 -DNDEBUG
	LDFLAGS := $(SDL_LIBS)
else
	$(error Unknown BUILD=$(BUILD))
endif

ifeq ($(PLATFORM),windows)
	CMD_MKDIR_P = powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(1))'"
	CMD_RM_RF = powershell -NoProfile -Command "Remove-Item -Recurse -Force -Path '$(subst /,\,$(1))'"
else
	CMD_MKDIR_P = mkdir -p "$(1)"
	CMD_RM_RF = rm -rf "$(1)"
endif

ifeq ($(PLATFORM),posix)
	SRCS := $(shell find $(SRC_DIR) -name "*.c")
else
	SRCS := $(shell powershell -NoProfile -Command "Get-ChildItem -Path '$(SRC_DIR)' -Recurse -Include *.c | ForEach-Object { $_.FullName -replace '\\\\','/' }")
endif

OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_ROOT_DIR)/%.o,$(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.c,$(DEP_ROOT_DIR)/%.d,$(SRCS))

.PHONY: all dirs clean run

all: dirs $(TARGET)

dirs:
	@$(call CMD_MKDIR_P,$(BIN_DIR))

$(TARGET): $(OBJS) | dirs
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_ROOT_DIR)/%.o: $(SRC_DIR)/%.c
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(call CMD_MKDIR_P,$(dir $(DEP_ROOT_DIR)/$*))
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_ROOT_DIR)/$*.d -c $< -o $@

run: all
	$(TARGET)

-include $(DEPS)

clean:
	@$(call CMD_RM_RF,build)
	@$(call CMD_RM_RF,out)
