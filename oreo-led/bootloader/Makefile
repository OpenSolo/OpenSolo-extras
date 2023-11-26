
# project config
OBJECT_DIR=build
SRC_DIR=src
INCLUDE_DIR=include
OUTPUT_NAME=oreoled-bootloader
DEVICE=attiny88
OBJECTS=${OBJECT_DIR}/twi_manager.o
OBJECTS+= ${OBJECT_DIR}/node_manager.o ${OBJECT_DIR}/boot_manager.o

BOOTLOADER_START=0x1800

# shell commands
SHELL_UTILS_DIR=${AVRSTUDIO_EXE_PATH}/shellutils
CP=cp
MKDIR=${SHELL_UTILS_DIR}/mkdir -p
RM=rm -f
MV=mv

# build commands
#PROGRAMMER=dragon_isp
#PROGRAMMER=jtag3isp -B 8.0
PROGRAMMER=avrisp2
PROG=avrdude -c ${PROGRAMMER} -p attiny88
AVROBJCOPY=avr-objcopy
AVRSIZE=avr-size
AVRGCC=avr-gcc
CFLAGS=-Wall -Wpadded -fdata-sections -ffunction-sections -Os -DF_CPU=8000000 -mmcu=${DEVICE} -Iinclude -DBOOTLOADER_START=${BOOTLOADER_START}
LDFLAGS=-Wl,--section-start=.text=${BOOTLOADER_START}

##############################################
# High level directives
##############################################

all:
	${MKDIR} ${OBJECT_DIR}
	make ${OBJECT_DIR}/${OUTPUT_NAME}.hex

flash: all
	${PROG} -U flash:w:${OBJECT_DIR}/${OUTPUT_NAME}.hex:i

clean:
	${RM} -r ${OBJECT_DIR}

# sets high speed (full rate) clock: 8MHz
fuse:
	${PROG} -u -U lfuse:w:0xEE:m -U hfuse:w:0xDD:m -U efuse:w:0xfe:m

##############################################
# Compile
##############################################

${OBJECT_DIR}/twi_manager.o: ${SRC_DIR}/twi_manager.c ${INCLUDE_DIR}/twi_manager.h
	${AVRGCC} ${CFLAGS} -c -g -Wa,-a,-ad ${SRC_DIR}/twi_manager.c -o ${OBJECT_DIR}/twi_manager.o > ${OBJECT_DIR}/twi_manager.s

${OBJECT_DIR}/node_manager.o: ${SRC_DIR}/node_manager.c ${INCLUDE_DIR}/node_manager.h
	${AVRGCC} ${CFLAGS} -c -g -Wa,-a,-ad ${SRC_DIR}/node_manager.c -o ${OBJECT_DIR}/node_manager.o > ${OBJECT_DIR}/node_manager.s

${OBJECT_DIR}/boot_manager.o: ${SRC_DIR}/boot_manager.c ${INCLUDE_DIR}/boot_manager.h
	${AVRGCC} ${CFLAGS} -c -g -Wa,-a,-ad ${SRC_DIR}/boot_manager.c -o ${OBJECT_DIR}/boot_manager.o > ${OBJECT_DIR}/boot_manager.s

${OBJECT_DIR}/main.o: ${SRC_DIR}/main.c ${OBJECTS} ${OBJECT_DIR}/node_manager.o
	${AVRGCC} ${CFLAGS} -c -g -Wa,-a,-ad ${SRC_DIR}/main.c -o ${OBJECT_DIR}/main.o > ${OBJECT_DIR}/main.s

##############################################
# Link
##############################################

${OBJECT_DIR}/${OUTPUT_NAME}.elf: ${OBJECT_DIR}/main.o ${OBJECTS}
	${AVRGCC} ${OBJECT_DIR}/main.o ${OBJECTS} ${CFLAGS} ${LDFLAGS} -o ${OBJECT_DIR}/${OUTPUT_NAME}.elf

${OBJECT_DIR}/${OUTPUT_NAME}.hex: ${OBJECT_DIR}/${OUTPUT_NAME}.elf
	${RM} -f ${OBJECT_DIR}/${OUTPUT_NAME}.hex
	${RM} -f ${OBJECT_DIR}/${OUTPUT_NAME}.bin
	${RM} -f ${OBJECT_DIR}/../${OUTPUT_NAME}.elf
	${CP} ${OBJECT_DIR}/${OUTPUT_NAME}.elf ${OBJECT_DIR}/../${OUTPUT_NAME}.elf
	${AVROBJCOPY} -j .text -j .data -j .bootreset -O ihex ${OBJECT_DIR}/${OUTPUT_NAME}.elf ${OBJECT_DIR}/${OUTPUT_NAME}.hex
	${AVRSIZE} --format=avr --mcu=${DEVICE} ${OBJECT_DIR}/${OUTPUT_NAME}.elf
