# Target library
lib := libfs.a

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

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

$(lib): $(objs)
	@echo "AR $@"
	$(Q)ar rcs $(lib) $(objs)

clean:
	@echo "CLEAN"
	$(Q)rm -f $(objs) $(deps)
