# Directory paths
PROGRAM1_SRC_DIR = ./src/pennfat
PROGRAM1_INC_DIR = ./src/pennfat
PROGRAM1_OBJ_DIR = ./bin/pennfat_bin

PROGRAM2_SRC_DIR = ./src/pennos
PROGRAM2_INC_DIR = ./src/pennos
PROGRAM2_OBJ_DIR = ./bin/pennos_bin

# Compiler and compiler flags
CC = clang 
# CFLAGS = -Wall -Werror -g
CFLAGS = -Wall -g

# Source files
PROGRAM1_SRC_FILES = $(wildcard $(PROGRAM1_SRC_DIR)/*.c)
PROGRAM2_SRC_FILES = $(wildcard $(PROGRAM2_SRC_DIR)/*.c)

# Object files
PROGRAM1_OBJ_FILES = $(patsubst $(PROGRAM1_SRC_DIR)/%.c, $(PROGRAM1_OBJ_DIR)/%.o, $(PROGRAM1_SRC_FILES))
PROGRAM2_OBJ_FILES = $(patsubst $(PROGRAM2_SRC_DIR)/%.c, $(PROGRAM2_OBJ_DIR)/%.o, $(PROGRAM2_SRC_FILES))

# Target executables
PROGRAM1_TARGET = ./bin/pennfat
PROGRAM2_TARGET = ./bin/pennos

all: $(PROGRAM1_TARGET) $(PROGRAM2_TARGET)

# Compilation rules
$(PROGRAM1_OBJ_DIR)/%.o: $(PROGRAM1_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(PROGRAM1_INC_DIR) -c $< -o $@

$(PROGRAM1_TARGET) : $(PROGRAM1_OBJ_FILES)
	$(CC) -lm -o $@ $^

$(PROGRAM2_OBJ_DIR)/%.o: $(PROGRAM2_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(PROGRAM2_INC_DIR) -c $< -o $@

$(PROGRAM2_TARGET) : $(PROGRAM2_OBJ_FILES)
	$(CC) -lm -o $@ $^

# Phony targets
.PHONY: clean

clean:
	rm -f $(PROGRAM1_OBJ_DIR)/*.o $(PROGRAM1_TARGET) $(PROGRAM2_OBJ_DIR)/*.o $(PROGRAM2_TARGET)

# Default target
all: $(PROGRAM1_TARGET) $(PROGRAM2_TARGET)

.PHONY: all
