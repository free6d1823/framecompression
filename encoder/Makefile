DEST=../bin
TARGET=afbcenc


CFLAGS=-g -O2 -Wall -pedantic -std=c99 -lm

SOURCES=afbc_common.c afbc_encode.c afbcenc_main.c afbcenc_util.c

all: $(TARGET)
$(TARGET): $(SOURCES)
	gcc $^ $(CFLAGS) -o $@ 
	mv $(TARGET) $(DEST)/. 

clean:
	rm -rf $(DEST)/$(TARGET)

