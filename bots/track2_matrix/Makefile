SRCDIR = src
OBJDIR = build
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(addprefix $(OBJDIR)/, $(notdir $(SRCS:.c=.o)))

CC = cc
CFLAGS = -Wall -Wextra -Werror -g -fsanitize=address
LDFLAGS = -lm -Wl,--wrap=core_action_move -Wl,--wrap=core_action_attack -Wl,--wrap=core_action_transferGems
CLIENT_LIB_DIR ?= $(shell if [ -d /core ]; then echo /core; else echo $(CURDIR)/../../core_sys; fi)
INC = -I inc -I $(CLIENT_LIB_DIR)/inc
EXEC = bot

HEADERS = $(wildcard inc/*.h)

all: ensure_obj_dir $(EXEC)

run: all
	./$(EXEC)

$(EXEC): $(OBJS) $(CLIENT_LIB_DIR)/core_lib.a
	$(CC) $(CFLAGS) $(OBJS) $(CLIENT_LIB_DIR)/core_lib.a $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ $(INC)

ensure_obj_dir:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -rf $(EXEC)

re: fclean all

.PHONY: all run clean fclean re $(EXEC) ensure_obj_dir
