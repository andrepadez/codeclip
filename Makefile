# ===== Project Configuration =====
CC      := gcc
CFLAGS  := -Wall -O2 -D_XOPEN_SOURCE=700
TARGET  := codeclip

SRC_DIR := src
SRC     := $(SRC_DIR)/main.c \
           $(SRC_DIR)/process_file.c \
           $(SRC_DIR)/helpers.c \
           $(SRC_DIR)/config_manager.c \
           $(SRC_DIR)/clipboard.c \
					 $(SRC_DIR)/bfs_traversal.c
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


# === Embed default config files ===
DEFAULTS_DIR := defaults
GENERATED_DIR := src/generated
GENERATED_HEADERS := $(GENERATED_DIR)/config_default.h $(GENERATED_DIR)/ignore_default.h

$(GENERATED_DIR):
	mkdir -p $(GENERATED_DIR)

# Turn defaults/config.yaml -> src/generated/config_default.h
$(GENERATED_DIR)/config_default.h: $(DEFAULTS_DIR)/config.yaml | $(GENERATED_DIR)
	@echo "const char *DEFAULT_CONFIG_YAML = R\"EOF(" > $@ && cat $< >> $@ && echo ")EOF\";" >> $@

# Turn defaults/codeclipignore -> src/generated/ignore_default.h
$(GENERATED_DIR)/ignore_default.h: $(DEFAULTS_DIR)/codeclipignore | $(GENERATED_DIR)
	@echo "const char *DEFAULT_IGNORE_FILE = R\"EOF(" > $@ && cat $< >> $@ && echo ")EOF\";" >> $@

# Ensure generated headers exist before compiling config_manager
src/config_manager.o: $(GENERATED_HEADERS)

# Add include path
CFLAGS  := -Wall -O2 -D_XOPEN_SOURCE=700 -Isrc -Isrc/generated
