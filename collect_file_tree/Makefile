BIN_DIR=bin/
SRC_DIR=src/
INCLUDE_DIR=include/
CURRENT_DIR=./
DEBUG_DIR=debug/

BIN_MAIN=$(BIN_DIR)/collect_file_tree

CFLAGS+=-Wall -Werror -I$(INCLUDE_DIR)

INCLUDES=\
	$(INCLUDE_DIR)/file_tree_nodes.h

all: $(BIN_MAIN)

$(BIN_MAIN): $(BIN_DIR)/collect_file_tree.o \
	$(BIN_DIR)/file_tree.o \
	$(INCLUDES) $(BIN_DIR)
	$(CC) $(CFLAGS) $(wordlist 1,2,$^) -o $@

$(BIN_DIR)/collect_file_tree.o: $(SRC_DIR)/collect_file_tree.c \
	$(INCLUDES) $(BIN_DIR)
	$(CC) -c $(CFLAGS) $< -o $@
	
$(BIN_DIR)/file_tree.o: $(SRC_DIR)/file_tree.c \
	$(INCLUDES) $(BIN_DIR)
	$(CC) -c $(CFLAGS) $< -o $@ 

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(DEBUG_DIR)/debug_main: $(DEBUG_DIR)/collect_file_tree.o \
	$(DEBUG_DIR)/file_tree.o \
	$(INCLUDES) $(BIN_DIR)
	$(CC) -g $(CFLAGS) $(wordlist 1,2,$^) -o $@
	
$(DEBUG_DIR)/file_tree.o: $(SRC_DIR)/file_tree.c $(INCLUDES) $(DEBUG_DIR)
	$(CC) -g -c $(CFLAGS) $< -o $@

$(DEBUG_DIR)/collect_file_tree.o: $(SRC_DIR)/collect_file_tree.c $(INCLUDES) $(DEBUG_DIR)
	$(CC) -g -c $(CFLAGS) $< -o $@
	
$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

clean:
	rm -rf $(BIN_DIR)
	rm -rf $(DEBUG_DIR)

mem_check:
	valgrind --leak-check=yes $(BIN_MAIN) $(CURRENT_DIR)

.PHONY: clean mem_check
