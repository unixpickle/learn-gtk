CFLAGS=$(shell pkg-config --cflags --libs gtk+-3.0)

all: build build/button_catcher build/img_puzzle

build/button_catcher: button_catcher/main.c
	$(CC) -o $@ $^ $(CFLAGS)

build/img_puzzle: img_puzzle/main.c
	$(CC) -o $@ $^ $(CFLAGS)

build:
	mkdir build

clean:
	rm -rf build