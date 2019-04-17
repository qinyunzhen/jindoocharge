VPATH = json:mqtt:linux
TARGET = jindoomqtt
COBJS = $(filter-out irtest.o,$(patsubst %.c,%.o,$(wildcard *.c)))
mqttCOBJS = $(filter-out irtest.o,$(patsubst %.c,%.o,$(wildcard mqtt/*.c)))
jsonCOBJS = $(filter-out irtest.o,$(patsubst %.c,%.o,$(wildcard json/*.c)))
linuxCOBJS = $(filter-out irtest.o,$(patsubst %.c,%.o,$(wildcard linux/*.c)))
INC = -I./json -I./mqtt -I./linux
CC =arm-linux-gcc
all:$(TARGET)
$(TARGET):$(COBJS) $(mqttCOBJS) $(jsonCOBJS) $(linuxCOBJS)
	$(CC) -o $@ $^
$(COBJS):%.o:%.c
	$(CC) $(INC) -c $< -o $@
$(mqttCOBJS):%.o:%.c
	$(CC) $(INC) -c $< -o $@
$(jsonCOBJS):%.o:%.c
	$(CC) $(INC) -c $< -o $@
$(linuxCOBJS):%.o:%.c
	$(CC) $(INC) -c $< -o $@
clean:
	rm *.o $(TARGET)
	rm mqtt/*.o
	rm json/*.o
	rm linux/*.o
