
#CFLAGS=-std=gnu99 -pedantic -pedantic-errors -Wall -Werror -O3 
CFLAGS=-std=gnu99 -pedantic -pedantic-errors -Wall -Werror -O3 -g
#CFLAGS=-std=gnu99 -pedantic -pedantic-errors -Wall -Werror -O0 -g

CC=gcc

INC=-I"./src"
CLI_INC=-I"./cliSrc"
JNI_INC=-I/usr/lib/jvm/java/include/ -I/usr/lib/jvm/java/include/linux/ -I"./jniSrc" -I"./jniHeaders"

CLI_LIBS = -lrt -lpthread
#CLI_LIBS = -lrt -lpthread -lprofiler

CLI_OBJS += \
./bin/cli/common/graph/graph.o \
./bin/cli/common/graph/routing.o \
./bin/cli/common/graph/routeTable.o \
./bin/cli/common/graph/seqTail.o \
./bin/cli/common/graph/smer.o \
./bin/cli/common/graph/smerArray.o \
./bin/cli/common/graph/smerMap.o \
./bin/cli/common/io/fastqParser.o \
./bin/cli/common/mem/mem.o \
./bin/cli/common/mem/memDispenser.o \
./bin/cli/common/mem/memPackStack.o \
./bin/cli/common/task/task.o \
./bin/cli/common/task/taskIndexing.o \
./bin/cli/common/task/taskRouting.o \
./bin/cli/common/task/taskRoutingLookup.o \
./bin/cli/common/task/taskRoutingDispatch.o \
./bin/cli/common/util/bitMap.o \
./bin/cli/common/util/bitPacking.o \
./bin/cli/common/util/bloom.o \
./bin/cli/common/util/log.o \
./bin/cli/common/util/pack.o \
./bin/cli/common/util/smerImplicitTree.o \
./bin/cli/obj/main.o 

CLI_DEPS := $(CLI_OBJS:.o=.d)
-include $(CLI_DEPS)

JNI_OBJS += \
./bin/jni/common/graph/graph.o \
./bin/jni/common/graph/routing.o \
./bin/jni/common/graph/routeTable.o \
./bin/jni/common/graph/seqTail.o \
./bin/jni/common/graph/smer.o \
./bin/jni/common/graph/smerArray.o \
./bin/jni/common/graph/smerMap.o \
./bin/jni/common/io/fastqParser.o \
./bin/jni/common/mem/mem.o \
./bin/jni/common/mem/memDispenser.o \
./bin/jni/common/mem/memPackStack.o \
./bin/jni/common/task/task.o \
./bin/jni/common/task/taskIndexing.o \
./bin/jni/common/task/taskRouting.o \
./bin/jni/common/task/taskRoutingLookup.o \
./bin/jni/common/task/taskRoutingDispatch.o \
./bin/jni/common/util/bitMap.o \
./bin/jni/common/util/bitPacking.o \
./bin/jni/common/util/bloom.o \
./bin/jni/common/util/log.o \
./bin/jni/common/util/pack.o \
./bin/jni/common/util/smerImplicitTree.o \
./bin/jni/obj/JNI_graph.o 

JNI_DEPS := $(JNI_OBJS:.o=.d)
-include $(JNI_DEPS)

all: bin bin/LOGAN bin/libLOGAN.so | bin

bin: 
	mkdir -p bin
	mkdir -p bin/cli/obj
	mkdir -p bin/cli/common
	mkdir -p bin/cli/common/graph
	mkdir -p bin/cli/common/io
	mkdir -p bin/cli/common/mem
	mkdir -p bin/cli/common/task
	mkdir -p bin/cli/common/util
	mkdir -p bin/jni/obj
	mkdir -p bin/jni/common
	mkdir -p bin/jni/common/graph
	mkdir -p bin/jni/common/io
	mkdir -p bin/jni/common/mem
	mkdir -p bin/jni/common/task
	mkdir -p bin/jni/common/util
	
bin/LOGAN: $(CLI_OBJS) | bin  
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	${CC} $(CFLAGS) -o "bin/LOGAN" $(CLI_OBJS) $(CLI_LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

bin/libLOGAN.so: $(JNI_OBJS) | bin 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	${CC} $(CFLAGS) -shared -o "bin/libLOGAN.so" $(JNI_OBJS)  
	@echo 'Finished building target: $@'
	@echo ' '

bin/cli/common/%.o: ./src/%.c | bin
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	${CC} $(CFLAGS) $(INC) $(CLI_INC) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/cli/obj/%.o: ./cliSrc/%.c | bin
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	${CC} $(CFLAGS) $(INC) $(CLI_INC) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


bin/jni/common/%.o: ./src/%.c | bin
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	${CC} $(CFLAGS) $(INC) $(JNI_INC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/jni/obj/%.o: ./jniSrc/%.c | bin
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	${CC} $(CFLAGS) $(INC) $(JNI_INC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
#$(CLI_OBJS): src/*.h cliSrc/*.h
#
#$(JNI_OBJS): src/*.h jniSrc/*.h jniHeaders/*.h	
	
clean:
	rm -rf bin
		
	
