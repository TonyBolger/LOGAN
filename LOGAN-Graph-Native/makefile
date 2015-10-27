
CFLAGS=-std=gnu99 -pedantic -pedantic-errors -Wall -Werror



INC=-I"./src"
CLI_INC=-I"./cliSrc"
JNI_INC=-I/usr/lib/jvm/java/include/ -I/usr/lib/jvm/java/include/linux/ -I"./jniSrc" -I"./jniHeaders"

CLI_LIBS := -lrt

CLI_OBJS += \
./bin/cli/common/graph.o \
./bin/cli/obj/main.o 

JNI_OBJS += \
./bin/jni/common/graph.o \
./bin/jni/obj/JNI_graph.o 

all: dirs bin/LOGAN bin/libLOGAN.so

dirs: 
	mkdir -p bin
	mkdir -p bin/cli/obj
	mkdir -p bin/cli/common
	mkdir -p bin/jni/obj
	mkdir -p bin/jni/common
	
bin/LOGAN: $(CLI_OBJS) 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -pg -o "bin/LOGAN" $(CLI_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

bin/libLOGAN.so: $(JNI_OBJS) 
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -shared -p -o "bin/libLOGAN.so" $(JNI_OBJS)  
	@echo 'Finished building target: $@'
	@echo ' '

bin/cli/common/%.o: ./src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(CLI_INC) -O3 -pg -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/cli/obj/%.o: ./cliSrc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(CLI_INC) -O3 -pg -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/jni/common/%.o: ./src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(JNI_INC) -O0 -g3 -p -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

bin/jni/obj/%.o: ./jniSrc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(CFLAGS) $(INC) $(JNI_INC) -O0 -g3 -p -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
