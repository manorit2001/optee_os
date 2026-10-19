# SPDX-License-Identifier: BSD-2-Clause
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
O ?= .
HOSTCC ?= cc
CPPFLAGS += -DCFG_TRANSFER_LIST=1 '-DUINTPTR_C(x)=x\#\#UL' \
	'-DU(x)=x\#\#U' '-DUL(x)=x\#\#UL' \
	-I$(ROOT)/core/kernel/tests/include -I$(ROOT)/core/include \
	-I$(ROOT)/lib/libutils/ext/include
CFLAGS += -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

.PHONY: all check
all: $(O)/test_transfer_list
check:: all
	$(O)/test_transfer_list

$(O)/test_transfer_list: $(ROOT)/core/kernel/tests/test_transfer_list_validate.c \
		$(ROOT)/core/kernel/transfer_list.c
	@mkdir -p $(@D)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@
