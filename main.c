#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

unsigned char mem[0xFF] = {0};
unsigned char reg[0x0F] = {0};
unsigned char progc = 0;

struct instruction {
    unsigned char reg    : 4; // Low nib
    unsigned char opcode : 4; // High nib
    union {
        struct {
            unsigned char b : 4; // Low nib
            unsigned char a : 4; // High nib
        } nibble;
        unsigned char byte : 8;
    } args;
} latest_instruct;

int step();

char p_nib(unsigned char num) {
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
    if (read(fd, mem, 0xFF) == -1) return EXIT_FAILURE;
    close(fd);
    while (step());
    putchar('\n');
    return EXIT_SUCCESS;
}

int step() {
    memcpy(&latest_instruct, &mem[progc], sizeof(struct instruction));
    //fprintf(stderr, "CODE: %02hhX %02hhX\n", *(unsigned char*)&latest_instruct, *((unsigned char*)&latest_instruct + 1));
    //fprintf(stderr, "OP: %c\n", p_nib(latest_instruct.opcode));
    //fprintf(stderr, "RG: %c\n", p_nib(latest_instruct.reg));
    //fprintf(stderr, "BYTE: %02hhx\n", latest_instruct.args.byte);
    switch (latest_instruct.opcode) {
        case 0x1: // Load
            reg[latest_instruct.reg] =
                mem[latest_instruct.args.byte];
            fprintf(stderr, "LOD REG %c WITH VALUE AT %02hhX (%02hhX)\n", p_nib(latest_instruct.reg), latest_instruct.args.byte, mem[latest_instruct.args.byte]);
            break;
        case 0x2: // Load
            reg[latest_instruct.reg] = 
                latest_instruct.args.byte;
            fprintf(stderr, "LOD REG %c WITH %02hhX\n", p_nib(latest_instruct.reg), latest_instruct.args.byte);
            break;
        case 0x3: // Store
            fprintf(stderr, "STR REG %c (%02hhX) TO %02hhX\n", p_nib(latest_instruct.reg), reg[latest_instruct.reg], latest_instruct.args.byte);
            mem[latest_instruct.args.byte] = 
                reg[latest_instruct.reg];
            break;
        case 0x4: // Move
            fprintf(stderr, "MOV REG %c(%02hhX) TO REG %c(%02hhX)\n", 
                    p_nib(latest_instruct.args.nibble.a),
                    reg[latest_instruct.args.nibble.a],
                    p_nib(latest_instruct.args.nibble.b),
                    reg[latest_instruct.args.nibble.b]);
            reg[latest_instruct.args.nibble.b] = 
                reg[latest_instruct.args.nibble.a];
            break;
        case 0x5: // Add
            fprintf(stderr, "ADD REG %c = REG %c(%02hhX) + REG %c(%02hhX) = %02hhX\n", 
                    p_nib(latest_instruct.reg),
                    p_nib(latest_instruct.args.nibble.a),
                    reg[latest_instruct.args.nibble.a],
                    p_nib(latest_instruct.args.nibble.b),
                    reg[latest_instruct.args.nibble.b],
                    (unsigned char)(reg[latest_instruct.args.nibble.a] + reg[latest_instruct.args.nibble.b]));
            reg[latest_instruct.reg] = 
                reg[latest_instruct.args.nibble.a] + 
                reg[latest_instruct.args.nibble.b];
            break;
        case 0x7: // Or
            reg[latest_instruct.reg] = 
                reg[latest_instruct.args.nibble.a] | 
                reg[latest_instruct.args.nibble.b];
            break;
        case 0x8: // And
            reg[latest_instruct.reg] = 
                reg[latest_instruct.args.nibble.a] & 
                reg[latest_instruct.args.nibble.b];
            break;
        case 0x9: // Xor
            reg[latest_instruct.reg] = 
                reg[latest_instruct.args.nibble.a] != 
                reg[latest_instruct.args.nibble.b];
            break;
        case 0xA: // Rotate right
            reg[latest_instruct.reg] = 
                reg[latest_instruct.reg] >> 
                latest_instruct.args.nibble.b;
            break;
        case 0xB: // Jump
            fprintf(stderr, "JMP IF REG %c(%02hhX) == 0(%02hhX) TO %02hhX\n", 
                    p_nib(latest_instruct.reg),
                    reg[latest_instruct.reg],
                    reg[0], reg[latest_instruct.args.byte]);
            if (reg[latest_instruct.reg] == reg[0x0])
                progc = latest_instruct.args.byte - 0x02; //Skip the increment!
            break;
        case 0x0: // Halt
        case 0xC: // Halt
            fprintf(stderr, "Halted.\n");
            return 0;
            break;
        case 0xD: //Special, output to display
            fprintf(stderr, "PUT ");
            putchar(reg[latest_instruct.reg]);
            fflush(stdout);
            fprintf(stderr, "\n");
            break;
        default:
            fprintf(stderr, "Invalid opcode: %X\n", latest_instruct.opcode);
            exit(EXIT_FAILURE);
    }
    progc += sizeof(struct instruction);
    if (progc > 0xF0) return 0;
    return 1;
}
