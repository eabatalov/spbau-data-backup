BIN_DIR=bin/
SRC_DIR=src/
INCLUDE_DIR=include/
CURRENT_DIR=./
DEBUG_DIR=debug/

CFLAGS+=-Wall -Werror -I$(INCLUDE_DIR)

INCLUDES=\
	$(INCLUDE_DIR)/file_tree_nodes.h

all: $(BIN_DIR)/file_tree.o
	
$(BIN_DIR)/file_tree.o: $(SRC_DIR)/file_tree.c \
	$(INCLUDES) $(BIN_DIR)
	$(CC) -c $(CFLAGS) $< -o $@ 

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

#debug: $(DEBUG_DIR)/file_tree.o

$(DEBUG_DIR)/file_tree.o: $(SRC_DIR)/file_tree.c $(INCLUDES) $(DEBUG_DIR)
	$(CC) -Wall -g -c $(CFLAGS) $< -o $@
	
$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

clean:
	rm -rf $(BIN_DIR)
	rm -rf $(DEBUG_DIR)

.PHONY: clean
