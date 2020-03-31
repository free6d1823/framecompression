DEST=../bin
TARGET=afbcgen
CFLAGS=-g -O2 -Wall -pedantic -std=c99 -lm

SOURCES=afbc_common.c afbc_encode.c afbc_generate.c afbcgen_main.c
all: $(TARGET)
$(TARGET): $(SOURCES)
	gcc $^ $(CFLAGS) -o $@ 
	mv $(TARGET) $(DEST)/. 

clean:
	rm -rf $(DEST)/$(TARGET)

