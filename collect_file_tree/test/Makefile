LIB_SO_NAME=fstree
LIB_NAME=lib$(LIB_SO_NAME)
LIB_STATIC=$(LIB_NAME).a
LIB_SHARED=$(LIB_NAME).so

BIN_DIR=./bin/
RELEASE_DIR=$(BIN_DIR)/release/
DEBUG_DIR=$(BIN_DIR)/debug/
SRC_DIR=src/

INTERFACE_INCLUDE_DIR=../include/

CFLAGS+=-Wall -Werror -ggdb \
	-I$(INTERFACE_INCLUDE_DIR)

INCLUDES=\
	$(INTERFACE_INCLUDE_DIR)/fs_tree.h \

ifneq ($(DEBUG),)
	TARGET_DIR+=$(DEBUG_DIR)
	LIB_DIR=../bin/debug/
else
	TARGET_DIR+=$(RELEASE_DIR)
	LIB_DIR=../bin/release/
endif
CFLAGS+=-L$(LIB_DIR)

BIN_FILES=$(TARGET_DIR)/first_test $(TARGET_DIR)/collect_file_tree
LIBS=$(LIB_DIR)/$(LIB_NAME)

all: $(BIN_FILES) scripts

$(TARGET_DIR)/first_test: $(SRC_DIR)/first_test.c \
	$(LIBS) \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) $(CFLAGS) $< -o $@ -l$(LIB_SO_NAME)

$(TARGET_DIR)/collect_file_tree: $(SRC_DIR)/collect_file_tree.c \
	$(LIBS) \
	$(INCLUDES) $(TARGET_DIR)
	$(CC) $(CFLAGS) $< -o $@ -l$(LIB_SO_NAME)

scripts: $(TARGET_DIR)
	cp $(SRC_DIR)/run_test.sh $(TARGET_DIR)

$(LIBS):
	make -C .. DEBUG=$(DEBUG)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean scripts
