CC ?= gcc
CFLAGS ?= -std=c17 -g -D_POSIX_SOURCE -D_DEFAULT_SOURCE -Wall -Werror -pedantic -I.
DEPFLAGS ?= -MMD -MP

TEST_DIR := test
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

BACKEND_SRCS := simulation.c io_utils.c
BACKEND_OBJS := $(addprefix $(OBJ_DIR)/,$(BACKEND_SRCS:.c=.o))
GOL_MAIN_OBJS := $(BACKEND_OBJS) $(OBJ_DIR)/main.o

TEST_EXECUTABLES := \
	$(BUILD_DIR)/$(TEST_DIR)/GOL_sim_tests \
	$(BUILD_DIR)/$(TEST_DIR)/GOL_io_tests

TEST_OBJS := \
	$(OBJ_DIR)/$(TEST_DIR)/GOL_sim_tests.o \
	$(OBJ_DIR)/$(TEST_DIR)/GOL_io_tests.o

DEPS := $(patsubst %.o,%.d,$(BACKEND_OBJS) $(GOL_MAIN_OBJS) $(TEST_OBJS))

all: gol libgol.so test

gol: $(BUILD_DIR)/gol
	@:

libgol.so: $(BUILD_DIR)/libgol.so
	@:

test: $(TEST_EXECUTABLES)
	@:

$(BUILD_DIR)/gol: $(GOL_MAIN_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/libgol.so: $(BACKEND_SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -shared -fPIC $^ -o $@

$(BUILD_DIR)/$(TEST_DIR)/GOL_sim_tests: $(OBJ_DIR)/$(TEST_DIR)/GOL_sim_tests.o $(BACKEND_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/$(TEST_DIR)/GOL_io_tests: $(OBJ_DIR)/$(TEST_DIR)/GOL_io_tests.o $(BACKEND_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o *.so *.exe gol
	rm -f $(TEST_DIR)/*.o

.PHONY: all clean gol libgol.so test

-include $(DEPS)
