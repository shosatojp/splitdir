CC=gcc
COPT=-g -O2 -lm

PREFIX=/usr/bin
TARGET=splitdir
SRC=split.c
OBJ=$(SRC:%.c=%.o)

all:$(TARGET)

%.o:%.c
	$(CC) $(COPT) -c $^ -o $@

$(TARGET):$(OBJ)
	$(CC) $^ $(COPT) -o $@

install:$(TARGET)
	cp -i $(TARGET) $(PREFIX)/

clean:
	rm -f *.o $(TARGET)