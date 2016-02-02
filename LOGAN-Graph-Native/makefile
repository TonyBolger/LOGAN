
CFLAGS=-std=gnu99 -pedantic -pedantic-errors -Wall -Werror -g



INC=-I"./src"
CLI_INC=-I"./cliSrc"
JNI_INC=-I/usr/lib/jvm/java/include/ -I/usr/lib/jvm/java/include/linux/ -I"./cliSrc" -I"./jniSrc" -I"./jniHeaders"

CLI_LIBS = -lrt -lpthread

CLI_OBJS += \
./bin/cli/common/graph.o \
./bin/cli/common/log.o \
./bin/cli/common/mem.o \
./bin/cli/common/pack.o \
./bin/cli/common/smer.o \
./bin/cli/common/smerMap.o \
./bin/cli/common/task.o \
./bin/cli/common/taskIndexing.o \
./bin/cli/common/taskRouting.o \
./bin/cli/obj/fastqParser.o \
./bin/cli/obj/main.o 

CLI_DEPS := $(CLI_OBJS:.o=.d)
-include $(CLI_DEPS)

JNI_OBJS += \
./bin/jni/common/graph.o \
./bin/jni/common/log.o \
./bin/jni/common/mem.o \
./bin/jni/common/pack.o \
./bin/jni/common/smer.o \
./bin/jni/common/smerMap.o \
./bin/jni/common/task.o \
./bin/jni/common/taskIndexing.o \
./bin/jni/common/taskRouting.o \
./bin/jni/obj/JNI_graph.o 

JNI_DEPS := $(JNI_OBJS:.o=.d)
-include $(JNI_DEPS)

all: bin/LOGAN bin/libLOGAN.so

bin: 
	mkdir -p bin
	
bin/cli: 	
	mkdir -p bin/cli/obj
	
bin/cli/common:	
	mkdir -p bin/cli/common
	
bin/jni/obj:	
	mkdir -p bin/jni/obj
	
bin/jni/common:	
	mkdir -p bin/jni/common
	
bin/LOGAN: $(CLI_OBJS) | bin bin/cli/obj bin/cli/common 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -pg -o "bin/LOGAN" $(CLI_OBJS) $(CLI_LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

bin/libLOGAN.so: $(JNI_OBJS) | bin bin/jni/obj bin/jni/common
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -shared -p -o "bin/libLOGAN.so" $(JNI_OBJS)  
	@echo 'Finished building target: $@'
	@echo ' '

bin/cli/common/%.o: ./src/%.c 
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(CLI_INC) -O3 -pg -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/cli/obj/%.o: ./cliSrc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(CLI_INC) -O3 -pg -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


bin/jni/common/%.o: ./src/%.c 
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(JNI_INC) -O0 -g3 -p -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/jni/obj/%.o: ./jniSrc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(JNI_INC) -O0 -g3 -p -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
#$(CLI_OBJS): src/*.h cliSrc/*.h
#
#$(JNI_OBJS): src/*.h jniSrc/*.h jniHeaders/*.h	
	
clean:
	rm $(CLI_OBJS) $(JNI_OBJS) bin/LOGAN bin/libLOGAN.so
		
	
