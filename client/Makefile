CLIENT_BIN=client
COMMON_DIR=../common/

all: $(CLIENT_BIN)

$(CLIENT_BIN): client.c $(COMMON_DIR)/protocol.h
	gcc -Wall -Werror -I$(COMMON_DIR) client.c -o $(CLIENT_BIN)
	
clean:
	rm $(CLIENT_BIN)

.PHONY: clean
