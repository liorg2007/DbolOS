ASM = nasm
LD = ld
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386

TARGET = dummy
SRC = dummy.asm
OBJ = dummy.o
OUT = dummy.out

all: $(OUT)

$(OUT): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $<

$(OBJ): $(SRC)
	$(ASM) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJ) $(OUT)
