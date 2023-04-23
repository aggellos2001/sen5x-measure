# common_sources = sensirion_config.h sensirion_common.h sensirion_common.c
# i2c_sources = sensirion_i2c_hal.h sensirion_i2c.h sensirion_i2c.c
# sen5x_sources = sen5x_i2c.h sen5x_i2c.c
sensirion_sources = libs/sen*.c
toml_sources = libs/toml*
config_sources = libs/config*

i2c_implementation ?= libs/*

CFLAGS = -Os -Wall -fstrict-aliasing -Wstrict-aliasing=1 -Wsign-conversion -fPIC -I.

OBJDIR = bin

ifdef CI
    CFLAGS += -Werror
endif

.PHONY: all clean

all: bin bin/measure
	
bin/measure: measure.c
	#$(CC) $(CFLAGS) -o bin/$@ $(sensirion_sources) $(config_sources) -w $(toml_sources) measure.c
	$(CC) $(CFLAGS) -o $@ $(sensirion_sources) $(config_sources) -w $(toml_sources) measure.c

clean:
	$(RM) bin/measure

bin: 
	$(shell mkdir -p $(OBJDIR))

run: 
	cd bin && ./measure
