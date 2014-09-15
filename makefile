all: build

build: clean
	spark compile ./

install:
	file=$$(ls firmware_*.bin | head -n 1); \
	spark flash --usb $$file

clean:
	rm -rf firmware_*.bin
