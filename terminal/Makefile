TARGET = main
OBJ = $(TARGET).o
SRC = $(TARGET).c
OUT = $(TARGET).out
CC = i386-elf-gcc

GLIBCDIR = ../toolchains/newlib/i386-dbolos/lib
INCLUDEDIR = ../toolchains/newlib/i386-dbolos/include

CFLAGS = -g -I$(INCLUDEDIR)
LDFLAGS = -nostdlib -nostartfiles -static

STARTFILES = $(GLIBCDIR)/crt0.o `i386-elf-gcc --print-file-name=crtbegin.o`
ENDFILES = `i386-elf-gcc --print-file-name=crtend.o`
LIBGROUP = -Wl,--start-group $(GLIBCDIR)/libc.a -lgcc -Wl,--end-group

$(OUT): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(STARTFILES) $^ $(LIBGROUP) $(ENDFILES) 

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o *.~ $(OUT)