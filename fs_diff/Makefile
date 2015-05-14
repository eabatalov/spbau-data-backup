LIB_SO_NAME=fstree
LIB_NAME=lib$(LIB_SO_NAME)
LIB_STATIC=$(LIB_NAME).a
LIB_SHARED=$(LIB_NAME).so

BIN_DIR=./bin/
RELEASE_DIR=$(BIN_DIR)/release/
DEBUG_DIR=$(BIN_DIR)/debug/
SRC_DIR=src/

LIB_INCLUDE_DIR=../collect_file_tree/include/

CFLAGS+=-Wall -Werror \
	 -I$(LIB_INCLUDE_DIR)
DEBUG_FLAGS+=-ggdb
RELEASE_FLAGS+=-O2

INCLUDES=\
	$(LIB_INCLUDE_DIR)/fs_tree.h

ifneq ($(DEBUG),)
	TARGET_DIR+=$(DEBUG_DIR)
	LIB_DIR=../collect_file_tree/bin/debug/
	CFLAGS += $(DEBUG_FLAGS)
else
	TARGET_DIR+=$(RELEASE_DIR)
	LIB_DIR=../collect_file_tree/bin/release/
	CFLAGS += $(RELEASE_FLAGS)
endif
CFLAGS+=-L$(LIB_DIR)

BIN_FILES=$(TARGET_DIR)/fs_diff
LIBS=$(LIB_DIR)/$(LIB_NAME)

all: $(BIN_FILES) 

$(TARGET_DIR)/fs_diff: $(SRC_DIR)/fs_diff.c \
	$(LIBS) \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) $(CFLAGS) $< -o $@ -l$(LIB_SO_NAME)

#scripts: $(TARGET_DIR)
#	cp $(SRC_DIR)/run_test.sh $(TARGET_DIR)

$(LIBS):
	make -C ../collect_file_tree DEBUG=$(DEBUG)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

#clean:
#	rm -rf $(BIN_DIR)
#
#.PHONY: clean
