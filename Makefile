CROSS_COMPILE = arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld -v
CP      = $(CROSS_COMPILE)objcopy
OD      = $(CROSS_COMPILE)objdump

LINKER_SCRIPT = stm32f100-picolibc.ld
CFLAGS  =  -I./ -fno-common -O0 -mcpu=cortex-m3 -mthumb -g -Wall -specs=picolibc.specs
LFLAGS  = -T$(LINKER_SCRIPT)
CPFLAGS = -Obinary
ODFLAGS = -S

all: test

clean:
	-rm -f *.lst *.o *.elf *.bin

test: test.elf
	@ echo "...copying"
	$(CP) $(CPFLAGS) test.elf test.bin
	$(OD) $(ODFLAGS) test.elf > test.lst

test.elf: test.c $(LINKER_SCRIPT)
	$(CC) $(CFLAGS) $(LFLAGS) test.c -o test.elf
