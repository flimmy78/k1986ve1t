##################################
# stm32 minimal example Makefile
##################################

AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
CP = arm-none-eabi-objcopy

LKR_SCRIPT = k1986ve1t.ld

IMAGE = $(shell pwd)/bue.elf

CFLAGS  = -c -fno-common -g -mcpu=cortex-m1 -mthumb -I../inc
LFLAGS  = -nostartfiles -T$(LKR_SCRIPT) 
LFLAGS  += -L/home/smolvik/gcc-arm-none-eabi-5_4-2016q3/lib/gcc/arm-none-eabi/5.4.1/armv6-m -L/home/smolvik/gcc-arm-none-eabi-5_4-2016q3/arm-none-eabi/lib/armv6-m
LIBS = --start-group -lgcc --end-group
CPFLAGS = -Obinary
AFLAGS = -g -mcpu=cortex-m1 -mthumb -mapcs-32 -ahls=bue.lst

all: bue.bin

adc_dma.o: adc_dma.c
	$(CC) $(CFLAGS) -O0 -o adc_dma.o adc_dma.c

regcur.o: regcur.c
	$(CC) $(CFLAGS) -O3 -o regcur.o regcur.c

startup.o: startup.c
	$(CC) $(CFLAGS) -O0 -o startup.o startup.c

cos_tb.o: cos_tb.c
	$(CC) $(CFLAGS) -O0 -o cos_tb.o cos_tb.c

main.o: main.c
	$(CC) $(CFLAGS) -O0 -o main.o main.c

bue.elf: main.o cos_tb.o startup.o adc_dma.o gdef.h regcur.o
	$(LD) $(LFLAGS) -Map test.map -o bue.elf main.o cos_tb.o startup.o adc_dma.o regcur.o $(LIBS)

bue.bin: bue.elf
	$(CP) $(CPFLAGS) bue.elf bue.bin
	arm-none-eabi-objdump -d bue.elf > main.s
clean:
	rm -rf *.o *.elf *.bin *.map

load: bue.elf
	../mdr1986x-JFlash-master/loadjlink $(IMAGE)

debug:
	arm-none-eabi-gdb
