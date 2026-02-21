# BltzNxt Transpiler Makefile
# Usage: make (from bltznxt directory)

# Detect OS
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
    # Preferred compiler for Windows (if using bundled tools)
    CXX ?= tools/mingw64/bin/g++.exe
    AR ?= tools/mingw64/bin/ar.exe
else
    EXE_EXT =
    CXX ?= g++
    AR ?= ar
endif

COMMON    = -std=c++17 -O2
CXXFLAGS  = $(COMMON) -Isrc/transpiler -include src/transpiler/std.h
LDFLAGS   ?= -static

SRC_DIR   = src/transpiler
BUILD_DIR = _build
TARGET    = $(BUILD_DIR)/bbc_cpp$(EXE_EXT)

SOURCES   = main toker parser decl declnode environ exprnode node prognode stmtnode type varnode cpp_generator memory
OBJECTS   = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(SOURCES)))

ifeq ($(OS),Windows_NT)
    MKDIR = @if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RM = @if exist $(subst /,\,$(1)) rd /s /q $(subst /,\,$(1))
    RM_F = @if exist $(subst /,\,$(1)) del /q $(subst /,\,$(1))
else
    MKDIR = mkdir -p $(1)
    RM = rm -rf $(1)
    RM_F = rm -f $(1)
endif

# Targets
all: $(TARGET) lib/libbbruntime.a

debug:
	@echo "OS: $(OS)"
	@echo "CXX: $(CXX)"
	@echo "SOURCES: $(SOURCES)"

$(BUILD_DIR)/%.o: src/transpiler/%.cpp
	$(call MKDIR,$(BUILD_DIR))
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/commands.def

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# =============================================================================
# Runtime Library
# =============================================================================
RT_SOURCES = $(wildcard src/runtime/*.cpp)
RT_OBJECTS = $(patsubst src/runtime/%.cpp,$(BUILD_DIR)/rt_%.o,$(RT_SOURCES))

$(BUILD_DIR)/rt_%.o: src/runtime/%.cpp
	$(call MKDIR,$(BUILD_DIR))
	$(CXX) $(COMMON) -DBB_RUNTIME_COMPILING -Isrc/runtime -c $< -o $@

lib/libbbruntime.a: $(RT_OBJECTS)
	$(call MKDIR,lib)
	$(AR) rcs $@ $^

clean:
	$(call RM,$(BUILD_DIR))
	$(call RM,lib)
	$(call RM_F,$(TARGET))
