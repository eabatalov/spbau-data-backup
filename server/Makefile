SERVER_BIN=server
COMMON_DIR=../common/

all: $(SERVER_BIN)

$(SERVER_BIN): server.c $(COMMON_DIR)/protocol.h
	gcc -Wall -Werror -I$(COMMON_DIR) server.c -o $(SERVER_BIN)
	
clean:
	rm $(SERVER_BIN)
