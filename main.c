#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

unsigned char mem[0x100] = {0};
unsigned char reg[0x10] = {0};
unsigned char program_counter = 0;

struct Instruction {
    unsigned char reg    : 4; // Low split
    unsigned char opcode : 4; // High split
    union {
        struct {
            unsigned char lo : 4; // Low split
            unsigned char hi : 4; // High split
        } split;
        unsigned char byte : 8;
    } args;
} instruction;

int step();

char nibble_to_hex(unsigned char num) {
    if (num <= 9) {
        return num + '0';
    } else {
        return num - 10 + 'A';
    }
}

int main (int argc, char** argv) {
    if (argc < 2) return EXIT_FAILURE;
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) return EXIT_FAILURE;
    if (read(fd, mem, 0x100) == -1) return EXIT_FAILURE;
    close(fd);
    while (step());
    putchar('\n');
    return EXIT_SUCCESS;
}

int step() {
    memcpy(&instruction, &mem[program_counter], sizeof(struct Instruction));

    fprintf(stderr, "\nCODE: %02hhX%02hhX", *(unsigned char*)&instruction, *((unsigned char*)&instruction + 1));
    fprintf(stderr, " PC: %02hhX", program_counter);
    fprintf(stderr, " OP: %c", nibble_to_hex(instruction.opcode));
    fprintf(stderr, " RG: %c", nibble_to_hex(instruction.reg));
    fprintf(stderr, " NIB: [A:%c, B:%c]", nibble_to_hex(instruction.args.split.hi), nibble_to_hex(instruction.args.split.lo));
    fprintf(stderr, " BYTE: %02hhx\n", instruction.args.byte);
    fprintf(stderr, "  ");

    switch (instruction.opcode) {
        case 0x1: // Load from location
            reg[instruction.reg] =
                mem[instruction.args.byte];
            fprintf(stderr, "Load reg %c with value at %02hhX (%02hhX)\n", nibble_to_hex(instruction.reg), instruction.args.byte, mem[instruction.args.byte]);
            break;
        case 0x2: // Load from adjacent
            reg[instruction.reg] = 
                instruction.args.byte;
            fprintf(stderr, "Load reg %c with %02hhX\n", nibble_to_hex(instruction.reg), instruction.args.byte);
            break;
        case 0x3: // Store to memory
            fprintf(stderr, "Store reg %c (%02hhX) to %02hhX\n", nibble_to_hex(instruction.reg), reg[instruction.reg], instruction.args.byte);
            mem[instruction.args.byte] = 
                reg[instruction.reg];
            break;
        case 0x4: // Move/Copy register 
            fprintf(stderr, "Move reg %c (%02hhX) to reg %c (%02hhX)\n", 
                    nibble_to_hex(instruction.args.split.hi),
                    reg[instruction.args.split.hi],
                    nibble_to_hex(instruction.args.split.lo),
                    reg[instruction.args.split.lo]);
            reg[instruction.args.split.lo] = 
                reg[instruction.args.split.hi];
            break;
        case 0x5: // Add
            fprintf(stderr, "Add reg %c (%02hhX) + reg %c (%02hhX) -> reg %c (%02hhX)\n", 
                    nibble_to_hex(instruction.args.split.hi),
                    reg[instruction.args.split.hi],
                    nibble_to_hex(instruction.args.split.lo),
                    reg[instruction.args.split.lo],
                    nibble_to_hex(instruction.reg),
                    (unsigned char)(reg[instruction.args.split.hi] + reg[instruction.args.split.lo]));
            reg[instruction.reg] = 
                reg[instruction.args.split.hi] + 
                reg[instruction.args.split.lo];
            break;
        case 0x7: // Or
            reg[instruction.reg] = 
                reg[instruction.args.split.hi] | 
                reg[instruction.args.split.lo];
            break;
        case 0x8: // And
            reg[instruction.reg] = 
                reg[instruction.args.split.hi] & 
                reg[instruction.args.split.lo];
            break;
        case 0x9: // Xor
            reg[instruction.reg] = 
                reg[instruction.args.split.hi] != 
                reg[instruction.args.split.lo];
            break;
        case 0xA: // Rotate right
            reg[instruction.reg] = 
                reg[instruction.reg] >> 
                instruction.args.split.lo;
            break;
        case 0xB: // Jump
            fprintf(stderr, "Jump to %02hhX\n if reg %c (%02hhX) == reg 0(%02hhX) ", 
                    nibble_to_hex(instruction.reg),
                    reg[instruction.args.byte],
                    reg[instruction.reg],
                    reg[0]);
            if (reg[instruction.reg] == reg[0x0])
                program_counter = instruction.args.byte - 0x02; //Skip the increment!
            break;
        case 0x0: // Halt
        case 0xC: // Halt
            fprintf(stderr, "Halted.\n");
            return 0;
            break;
        case 0xD: //Special, copy register to display
            fprintf(stderr, "Putchar from reg %c (%02hhX) [", nibble_to_hex(instruction.reg), reg[instruction.reg]);
            putchar(reg[instruction.reg]);
            fflush(stdout);
            fprintf(stderr, "]\n");
            break;
        case 0xE: //Compare/less
            fprintf(stderr, "Set reg %c (%02hhX) to reg zero if reg %c (%02hhX) < reg %c (%02hhX)\n", 
                    nibble_to_hex(instruction.reg),
                    reg[instruction.reg],
                    nibble_to_hex(instruction.args.split.hi),
                    reg[instruction.args.split.hi],
                    nibble_to_hex(instruction.args.split.lo),
                    reg[instruction.args.split.lo]);
            if (reg[instruction.args.split.hi] < reg[instruction.args.split.lo]) {
                reg[instruction.reg] = reg[0];
                fprintf(stderr, "\tTrue, set reg to reg0.\n");
            } else {
                fprintf(stderr, "\tFalse\n");
            }
            break;
        case 0xF: //Debug register value
            fprintf(stderr, "Debug reg %c [ ", nibble_to_hex(instruction.reg));
            printf("0x%02hhX ", reg[instruction.reg]);
            fflush(stdout);
            fprintf(stderr, " ]\n");
            break;
        default:
            fprintf(stderr, "Invalid opcode: %X\n", instruction.opcode);
            exit(EXIT_FAILURE);
    }
    program_counter += sizeof(struct Instruction);
    if (program_counter > 0xF0) return 0;
    return 1;
}
