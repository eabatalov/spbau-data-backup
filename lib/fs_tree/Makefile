LIB_NAME=libfstree
LIB_STATIC=$(LIB_NAME).a
LIB_SHARED=$(LIB_NAME).so

BIN_DIR=./bin/
RELEASE_DIR=$(BIN_DIR)/release/
DEBUG_DIR=$(BIN_DIR)/debug/
SRC_DIR=src/

INTERFACE_INCLUDE_DIR=./include/
INTERNAL_INCLUDE_DIR=./src/

CFLAGS+=-Wall -Werror -fPIC \
	-I$(INTERFACE_INCLUDE_DIR) -I$(INTERNAL_INCLUDE_DIR)
RELEASE_FLAGS=-O2
DEBUG_FLAGS=-ggdb

INCLUDES=\
	$(INTERFACE_INCLUDE_DIR)/fs_tree.h \
	$(INTERNAL_INCLUDE_DIR)/inodes.h

ifneq ($(DEBUG),)
	TARGET_DIR+=$(DEBUG_DIR)
	CFLAGS+=$(DEBUG_FLAGS)
else
	TARGET_DIR+=$(RELEASE_DIR)
	CFLAGS+=$(RELEASE_FLAGS)
endif

OBJ_FILES=$(TARGET_DIR)/fs_tree.o $(TARGET_DIR)/dfs.o $(TARGET_DIR)/bfs.o $(TARGET_DIR)/inodes.o
LIBS=$(TARGET_DIR)/$(LIB_SHARED) $(TARGET_DIR)/$(LIB_STATIC)

all: $(LIBS)
	
$(TARGET_DIR)/$(LIB_SHARED): $(OBJ_FILES) $(TARGET_DIR)
	gcc -shared $(OBJ_FILES) -o $@

$(TARGET_DIR)/$(LIB_STATIC): $(OBJ_FILES) $(TARGET_DIR)
	ar rcs $@ $(OBJ_FILES)

$(TARGET_DIR)/fs_tree.o: $(SRC_DIR)/fs_tree.c \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) -c $(CFLAGS) $< -o $@ 

$(TARGET_DIR)/dfs.o: $(SRC_DIR)/dfs.c \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) -c $(CFLAGS) $< -o $@ 

$(TARGET_DIR)/bfs.o: $(SRC_DIR)/bfs.c \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) -c $(CFLAGS) $< -o $@ 


$(TARGET_DIR)/inodes.o: $(SRC_DIR)/inodes.c \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean
