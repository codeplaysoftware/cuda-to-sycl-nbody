CC=gcc
CXX=g++
MKDIR_P = mkdir -p
MV_F = mv -f

PREREQ_DIR=obj/prerequisites
OBJ_DIR=obj
SRC_DIRS=src
OUTPUT_DIR=bin

CFLAGS=-Wall -Iinclude/
CXXFLAGS=-W -std=c++11 -D_USE_MATH_DEFINES -Iinclude/

LIBS=glfw3

INCLUDE=`pkg-config glm $(LIBS) --cflags`
LDFLAGS=`pkg-config $(LIBS) --libs`

ifneq ($(OS),Windows_NT)
	LDFLAGS+= -ldl
endif

DEPFLAGS = -MT $$@ -MMD -MP -MF $(PREREQ_DIR)/$$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) -o $$@ -c $$< $(CFLAGS)
COMPILE.cc = $(CXX) $(DEPFLAGS) -g -o $$@ -c $$< $(CXXFLAGS)
POSTCOMPILE = $(MV_F) $(PREREQ_DIR)/$$*.Td $(PREREQ_DIR)/$$*.d
LINK_DEBUG = $(CXX) -g $$^ -o $$@ $(LDFLAGS)
LINK = $(CXX) $$^ -o $$@ $(LDFLAGS)

$(shell $(MKDIR_P) $(OBJ_DIR))
$(shell $(MKDIR_P) $(PREREQ_DIR))
$(shell $(MKDIR_P) $(OUTPUT_DIR))

rules=nbody

# DEPENDENCIES FOR BINARIES
nbody=glad shader gen sim_param gl camera
# DEPENDENCIES FOR SRC FILES

# DEBUG
DEBUG ?= 1

deps = $(1) $(foreach name,$($(1)),$(call deps,$(name)))

ifeq ($(DEBUG),1)
define make_rule
$(2): $(1)
$(1): $(foreach dep,$(call deps,$(2)),$(OBJ_DIR)/$(dep).o)
	@echo Building debug binary $$@...
	@$(LINK_DEBUG)
endef
else
define make_rule
$(2): $(1)
$(1): $(foreach dep,$(call deps,$(2)),$(OBJ_DIR)/$(dep).o)
	@echo Building release binary $$@...
	@$(LINK)
endef
endif

all: $(rules)

# generate rules for each binary
ifeq ($(OS),Windows_NT)
$(foreach name,$(rules),$(eval $(call make_rule,$(OUTPUT_DIR)/$(name).exe,$(name))))
else
$(foreach name,$(rules),$(eval $(call make_rule,$(OUTPUT_DIR)/$(name),$(name))))
endif

define gen_rule_cpp
obj/%.o: $(1)/%.cpp $(PREREQ_DIR)/%.d
	@echo Building cpp source $$<...
	@$(COMPILE.cc)
	@$(POSTCOMPILE)
endef

define gen_rule_c
obj/%.o: $(1)/%.c $(PREREQ_DIR)/%.d
	@echo Building c source $$<...
	@$(COMPILE.c)
	@$(POSTCOMPILE)
endef

$(foreach dir,$(SRC_DIRS),$(eval $(call gen_rule_cpp,$(dir))))
$(foreach dir,$(SRC_DIRS),$(eval $(call gen_rule_c,$(dir))))

clean:
	rm -f obj/*.o;
	rm -f $(PREREQ_DIR)/*.d

.PHONY: all clean $(rules)

$(PREREQ_DIR)/%.d: ;

.PRECIOUS: $(PREREQ_DIR)/%.d

-include $(patsubst %,$(PREREQ_DIR)/%.d,$(basename $(SRC_DIRS)))
