detected_OS := $(shell uname -s)

gcc-arm-none-eabi-5_4-2016q3:
ifeq ($(MSYSTEM), MINGW64)
	sh get_gcc_msys.sh
endif
ifeq ($(detected_OS), Darwin)
	sh get_gcc_osx.sh
endif


.PHONY: clean
clean:
	rm -rf gcc-arm-none-eabi-5_4-2016q3
