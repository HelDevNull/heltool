SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE1 := $(BIN_DIR)/heltool-mc
EXE2 := $(BIN_DIR)/heltool-u2t
EXE3 := $(BIN_DIR)/heltool-t2u

SRC1 := $(SRC_DIR)/heltool-mc.c
OBJ1 := $(SRC1:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

SRC2 := $(SRC_DIR)/heltool-u2t.c
OBJ2 := $(SRC2:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

SRC3 := $(SRC_DIR)/heltool-u2t.c
OBJ3 := $(SRC2:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)


CPPFLAGS := -Iinclude -MMD -MP
CFLAGS   := -Wall -pthread
LDFLAGS  := -Llib
LDLIBS   := 

.PHONY: all clean

all: $(EXE1) $(EXE2) $(EXE3)

$(EXE1): $(OBJ1) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(EXE2): $(OBJ2) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@


$(EXE3): $(OBJ3) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)

