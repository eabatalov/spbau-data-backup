BIN_DIR=bin/
INNER_BIN_DIR=src/bin/

SRC_DIR=src/
INNER_SRC_DIR=src/src/

INCLUDE_DIR=include/
INNER_INCLUDE_DIR=src/include/

CURRENT_DIR=./
DEBUG_DIR=debug/
INNER_DEBUG_DIR=src/debug/

BIN_MAIN=$(BIN_DIR)/main

CFLAGS+=-Wall -Werror -I$(INCLUDE_DIR) -I$(INNER_INCLUDE_DIR)

INCLUDES=\
	$(INCLUDE_DIR)/fs_tree.h

INNER_INCLUDES=\
	$(INNER_INCLUDE_DIR)/file_tree_nodes.h

all: $(BIN_MAIN)

$(BIN_MAIN): $(BIN_DIR)/main.o \
	$(BIN_DIR)/fs_tree.o \
	$(INNER_BIN_DIR)/file_tree.o \
	$(INNER_INCLUDES) \
	$(INCLUDES) $(BIN_DIR)
	$(CC) $(CFLAGS) $(wordlist 1,3,$^) -o $@

$(BIN_DIR)/main.o: $(SRC_DIR)/main.c \
	$(INCLUDES) $(BIN_DIR)
	$(CC) -c $(CFLAGS) $< -o $@
	
$(BIN_DIR)/fs_tree.o: $(SRC_DIR)/fs_tree.c \
	$(INNER_BIN_DIR)/file_tree.o \
	$(INCLUDES) \
	$(INNER_INCLUDES) $(BIN_DIR)
	$(CC) -c $(CFLAGS) $< -o $@ 

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(INNER_BIN_DIR)/file_tree.o: $(INNER_SRC_DIR)/file_tree.c \
	$(INNER_INCLUDES)
	$(MAKE) -C $(SRC_DIR)

debug: $(DEBUG_DIR)/debug_main

$(DEBUG_DIR)/debug_main: $(DEBUG_DIR)/main.o \
	$(INNER_DEBUG_DIR)/file_tree.o \
	$(DEBUG_DIR)/fs_tree.o \
	$(INCLUDES) $(BIN_DIR)
	$(CC) -Wall -g $(CFLAGS) $(wordlist 1,3,$^) -o $@
	
$(DEBUG_DIR)/fs_tree.o: $(SRC_DIR)/fs_tree.c $(INCLUDES) $(DEBUG_DIR)
	$(CC) -Wall -g -c $(CFLAGS) $< -o $@

$(DEBUG_DIR)/main.o: $(SRC_DIR)/main.c $(INCLUDES) $(DEBUG_DIR)
	$(CC) -Wall -g -c $(CFLAGS) $< -o $@
	
$(INNER_DEBUG_DIR)/file_tree.o: $(INNER_SRC_DIR)/file_tree.c $(INNER_INCLUDES)
	cd $(SRC_DIR) && $(MAKE) debug//file_tree.o
	
$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

clean:
	cd $(SRC_DIR) && $(MAKE) clean
	rm -rf $(BIN_DIR)
	rm -rf $(DEBUG_DIR)

mem_check: debug
	valgrind --leak-check=full --show-leak-kinds=all $(DEBUG_DIR)/debug_main $(CURRENT_DIR)

.PHONY: clean mem_check debug
