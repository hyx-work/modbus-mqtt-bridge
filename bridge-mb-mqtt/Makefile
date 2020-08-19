TARGET = bridge-mb-mqtt
CC = gcc

# -pedantic -O2
CFLAGS = -W -Wall -g  -O2 -I /usr/local/include/modbus
LIBS = -lm  -lmosquitto -lmodbus 

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(SUBDIRS)
	$(CC) $(OBJECTS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
