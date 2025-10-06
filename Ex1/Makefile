# Compiler and flags
CC = gcc

# Target name
TARGET = exercise1

# Source file
SRC = resolver.c

# === Default rule ===
all: $(TARGET)

# === Build rule ===
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

# === Run rule ===
run: $(TARGET)
	./$(TARGET) $(ARGS)

# === Clean rule ===
clean:
	rm -f $(TARGET)
