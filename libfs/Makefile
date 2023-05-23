# Target library
lib := libfs.a

targets := fs 
objs := fs.o disk.o 

CC := gcc
CFLAGS := -Wall -Wextra -Werror -MMD
CFLAGS += -g

ifneq ($(V), 1)
Q = @
endif

all: $(lib) $(targets)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

## TODO: Phase 1

fs: $(objs) 
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

$(lib): $(objs)
	@echo "AR $@"
	$(Q)ar rcs $(lib) $(objs)

clean:
	@echo "CLEAN"
	$(Q)rm -f $(targets) $(objs) $(deps)
