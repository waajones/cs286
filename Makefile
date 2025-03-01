CC = g++
CFLAGS = -Wall -O2
TARGET = mipssim
SRC = mipssim.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

