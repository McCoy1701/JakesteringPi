CC = gcc
CINC = -Iinclude
CFLAGS = -g -lpthread -lm 

BIN_DIR = bin
OBJ_DIR = obj
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
JAKESTERING_DIR = jakestering

.PHONY: all

all: $(BIN_DIR)

$(OBJ_DIR)/keypad.o: $(JAKESTERING_DIR)/keypad.c
	$(CC) $< -c $(CINC) -o $@

$(OBJ_DIR)/lcd.o: $(JAKESTERING_DIR)/lcd.c
	$(CC) $< -c $(CINC) -o $@

$(OBJ_DIR)/lcd128x64.o: $(JAKESTERING_DIR)/lcd128x64.c
	$(CC) $< -c $(CINC) -o $@

$(OBJ_DIR)/jakestering.o: $(JAKESTERING_DIR)/jakestering.c
	$(CC) $< -c $(CINC) -o $@

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) $< -c $(CINC) -o $@

$(BIN_DIR): always $(OBJ_DIR)/main.o $(OBJ_DIR)/jakestering.o $(OBJ_DIR)/lcd128x64.o $(OBJ_DIR)/lcd.o $(OBJ_DIR)/keypad.o
	$(CC) $(OBJ_DIR)/main.o $(OBJ_DIR)/jakestering.o $(OBJ_DIR)/lcd128x64.o  $(OBJ_DIR)/lcd.o $(OBJ_DIR)/keypad.o $(CFLAGS) -o $@/bin


.PHONY: build

build: $(BUILD_DIR)

$(BUILD_DIR): create $(JAKESTERING_DIR)/jakestering.c $(JAKESTERING_DIR)/lcd128x64.c $(JAKESTERING_DIR)/lcd.c $(JAKESTERING_DIR)/keypad.c 
	$(CC) -fPIC -shared $(JAKESTERING_DIR)/jakestering.c $(JAKESTERING_DIR)/lcd128x64.c $(JAKESTERING_DIR)/lcd.c $(JAKESTERING_DIR)/keypad.c $(CINC) $(CFLAGS) -o $@/libJakestering.so

.PHONY: install
install:
	sudo cp $(INC_DIR)/* /usr/include
	sudo cp $(BUILD_DIR)/libJakestering.so /usr/lib

.PHONY: uninstall
uninstall:
	sudo rm /usr/include/lcd.h
	sudo rm /usr/include/lcd128x64.h
	sudo rm /usr/include/keypad.h
	sudo rm /usr/include/jakestering.h
	sudo rm /usr/lib/libJakestering.so

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(BUILD_DIR)
	clear

.PHONY: always
always:
	mkdir -p $(BIN_DIR) $(OBJ_DIR)

.PHONY: create
create:
	mkdir -p $(BUILD_DIR) $(OBJ_DIR)
