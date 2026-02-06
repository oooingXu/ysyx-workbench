CSRCS += $(NPC_HOME)/csrc/memory/pmem.c
CSRCS += $(NPC_HOME)/csrc/device/device.c
CSRCS += $(NPC_HOME)/csrc/device/serial.c
CSRCS += $(NPC_HOME)/csrc/device/timer.c
CSRCS += $(NPC_HOME)/csrc/device/map.c
CSRCS += $(NPC_HOME)/csrc/device/mmio.c
CSRCS += $(NPC_HOME)/csrc/sdb/sdb.c
CSRCS += $(NPC_HOME)/csrc/sdb/watchpoint.c
CSRCS += $(NPC_HOME)/csrc/sdb/expr.c
CSRCS += $(NPC_HOME)/csrc/cpu/reg.c
CSRCS += $(NPC_HOME)/csrc/cpu/dut.c
CSRCS += $(NPC_HOME)/csrc/utils/time.c
CSRCS += $(NPC_HOME)/csrc/utils/ftrace.c
CSRCS += $(NPC_HOME)/csrc/utils/itrace.c
CSRCS += $(NPC_HOME)/csrc/utils/iringbuf.c
CSRCS += $(NPC_HOME)/csrc/lcommon/lightsss.cpp
CSRCS += $(NPC_HOME)/csrc/lcommon/lcommon.cpp
CSRCS += $(NPC_HOME)/csrc/counter/counter.c

# Konata pipeline visualization
ifdef CONFIG_KONATA
CSRCS += $(NPC_HOME)/csrc/konata/konata_logger.c
CSRCS += $(NPC_HOME)/csrc/konata/pipeline_tracker.c
CSRCS += $(NPC_HOME)/csrc/konata/konata_stage_tracker.c
CSRCS += $(NPC_HOME)/csrc/konata/dpi_interface.cpp
endif

INC_PATH += $(NPC_HOME)/csrc
INC_PATH += $(NPC_HOME)/csrc/counter
INC_PATH += $(NPC_HOME)/csrc/cpu
INC_PATH += $(NPC_HOME)/csrc/memory
INC_PATH += $(NPC_HOME)/csrc/sdb
INC_PATH += $(NPC_HOME)/csrc/device
INC_PATH += $(NPC_HOME)/csrc/utils
INC_PATH += $(NPC_HOME)/csrc/include
INC_PATH += $(NPC_HOME)/csrc/lcommon
INC_PATH += $(NPC_HOME)/include/generated
