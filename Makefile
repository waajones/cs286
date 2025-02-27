CC = g++
CFLAGS = -Wall -O2
TARGET = mipssim
SRC = decode.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
