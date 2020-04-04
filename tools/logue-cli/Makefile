detected_OS := $(shell uname -s)
logue-cli.exe:
	echo Fetching $(detected_OS) logue-cli executable...
ifeq ($(MSYSTEM), MINGW64)
	sh get_logue_cli_msys.sh
endif
ifeq ($(detected_OS), Darwin)
	sh get_logue_cli_osx.sh
endif


.PHONY: clean
clean:
	rm -f EULA.md logue-cli.exe
