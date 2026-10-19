# SPDX-License-Identifier: BSD-2-Clause
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../../../..)
include $(ROOT)/core/kernel/tests/Makefile
CPPFLAGS += -DPLATFORM_FLAVOR_am62x=1 \
	-I$(ROOT)/core/arch/arm/plat-k3 -I$(ROOT)/core/arch/arm/include \
	-I$(ROOT)/core/lib/libfdt/include -I$(ROOT)/lib/libutee/include

all: $(O)/test_ti_sci_handoff
check:: all
	$(O)/test_ti_sci_handoff

$(O)/test_ti_sci_handoff: $(ROOT)/core/arch/arm/plat-k3/tests/test_ti_sci_handoff.c \
		$(ROOT)/core/arch/arm/plat-k3/drivers/ti_sci_handoff.c \
		$(ROOT)/core/kernel/transfer_list.c \
		$(wildcard $(ROOT)/core/lib/libfdt/fdt*.c)
	@mkdir -p $(@D)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@
