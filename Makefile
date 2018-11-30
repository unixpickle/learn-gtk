CFLAGS=$(shell pkg-config --cflags --libs gtk+-3.0) -lm -lpthread

all: build build/button_catcher build/img_puzzle build/video_trim

build/button_catcher: button_catcher/main.c
	$(CC) -o $@ $^ $(CFLAGS)

build/img_puzzle: img_puzzle/main.c
	$(CC) -o $@ $^ $(CFLAGS)

build/video_trim: video_trim/main.c video_trim/video_info.c
	$(CC) -o $@ $^ $(CFLAGS) $(shell pkg-config --cflags --libs libavformat libavcodec) -Ivideo_trim

build:
	mkdir build

clean:
	rm -rf build