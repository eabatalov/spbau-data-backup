MAIN_BIN=collect_file_tree
HEADERS_DIR=../headers/

all: $(MAIN_BIN)

$(MAIN_BIN): collect_file_tree.o file_tree.o $(HEADERS_DIR)/file_tree_nodes.h
	gcc -Wall -Werror -I$(HEADERS_DIR) collect_file_tree.o -o $(MAIN_BIN)

collect_file_tree.o: collect_file_tree.c $(HEADERS_DIR)/file_tree_nodes.h
	gcc -c -Wall -Werror -I$(HEADERS_DIR) collect_file_tree.c -o collect_file_tree.o
	
file_tree.o: file_tree.c $(HEADERS_DIR)/file_tree_nodes.h
	gcc -c -Wall -Werror -I$(HEADERS_DIR) file_tree.c -o file_tree.o 

clean:
	rm *.o
	rm $(MAIN_BIN)
