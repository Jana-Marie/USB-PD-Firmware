# List of all the board related files.
BOARDSRC = $(CHIBIOS)/../boards/PD_BUDDY_SINK/board.c

# Required include directories
BOARDINC = $(CHIBIOS)/../boards/PD_BUDDY_SINK $(CHIBIOS)/../src

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
