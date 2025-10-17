# ===== Project Configuration =====
CC      := gcc
CFLAGS  := -Wall -O2 -D_XOPEN_SOURCE=700
TARGET  := codeclip

SRC_DIR := src
SRC     := $(SRC_DIR)/main.c \
           $(SRC_DIR)/process_file.c \
           $(SRC_DIR)/helpers.c \
           $(SRC_DIR)/config_manager.c
OBJS    := $(SRC:.c=.o)
DEPS    := $(SRC_DIR)/helpers.h $(SRC_DIR)/config_manager.h

# ===== Default Build =====
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@


# ===== Utilities =====
run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)/usr/local/bin
	cp $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall run
