# Building scripts

## Assemble 

nasm -f elf32 testfiles/proc1.asm -o testfiles/proc1.o
ld -m elf_i386 testfiles/proc1.o -o testfiles/proc1
