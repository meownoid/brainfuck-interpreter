#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_ENABLED 1

#define KiB 1024
#define MiB 1024 * KiB
#define GiB 1024 * MiB


const size_t MEM_CHUNK_SIZE = 1 * MiB;
const size_t MEM_MAX_SIZE = 1 * GiB;
const size_t FILE_MAX_SIZE = 1 * GiB;

typedef uint8_t mem_t;

mem_t *mem = NULL;
char *program = NULL;
size_t *loops_stack = NULL;

size_t mem_size = 0;
size_t program_size = 0;

size_t mem_ptr = 0;
size_t mem_max = 0;

size_t max_nested_loops = 0;
size_t loops_stack_ptr = 0;


void allocate_memory() {
    size_t new_size = mem_size + MEM_CHUNK_SIZE;

    if (new_size > MEM_MAX_SIZE) {
        fprintf(stderr, "OOM Error: can't allocate more than %zu bytes of memory\n", MEM_MAX_SIZE);
        exit(EXIT_FAILURE);
    }

    void *new_mem = realloc(mem, new_size);

    if (new_mem == NULL) {
        fprintf(stderr, "OOM Error: can't allocate %zu bytes of memory\n", new_size);
        exit(EXIT_FAILURE);
    }

    mem = (mem_t *) new_mem;
    memset(mem + mem_size, 0, new_size - mem_size);
    mem_size = new_size;
}

void read_program(char *file_name) {
    FILE *fd = fopen(file_name, "r");

    if (fd == NULL) {
        fprintf(stderr, "IO Error: unable to open %s\n", file_name);
        exit(EXIT_FAILURE);
    }

    fseek(fd, 0L, SEEK_END);
    size_t file_size = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    if (file_size > FILE_MAX_SIZE) {
        fprintf(stderr, "IO Error: file is too big (max: %zu bytes)\n", FILE_MAX_SIZE);
        exit(EXIT_FAILURE);
    }

    program = (char *) calloc(file_size + 1, sizeof(char));

    if (program == NULL) {
        fprintf(stderr, "OOM Error: can't allocate %lu bytes of memory\n", file_size * sizeof(char));
        exit(EXIT_FAILURE);
    }

    fread(program, sizeof(char), file_size, fd);
    fclose(fd);

    program_size = file_size;
}

void clean_check_program() {
    size_t k = 0;
    size_t brackets = 0;

    for (size_t i = 0; i < program_size; ++i) {
        char c = program[i];

        if (c == '>' || c == '<' || c == '+' || c == '-' ||
            c == ',' || c == '.' || c == '[' || c == ']' ||
            c == '@') {
            if (k != i) {
                program[k] = c;
            }

            ++k;

            if (brackets > max_nested_loops) {
                max_nested_loops = brackets;
            }

            if (c == '[') {
                ++brackets;
            } else if (c == ']') {
                --brackets;
            }
        }
    }

    if (brackets != 0) {
        fprintf(stderr, "Parsing Error: brackets didn't match\n");
        exit(EXIT_FAILURE);
    }

    loops_stack = (size_t *) calloc(max_nested_loops, sizeof(size_t));

    if (loops_stack == NULL) {
        fprintf(stderr, "OOM Error: can't allocate %zu bytes of memory\n", max_nested_loops * sizeof(size_t));
        exit(EXIT_FAILURE);
    }

    program_size = k;
}

void evaluate_program() {
    allocate_memory();

    size_t i = 0;

    while (i < program_size) {
        switch (program[i]) {
            case '>':
                ++mem_ptr;

                if (mem_ptr >= mem_size) {
                    allocate_memory();
                }

                ++i;
                break;

            case '<':
                if (mem_ptr == 0) {
                    fprintf(stderr, "MEM Error: negative pointer\n");
                    exit(EXIT_FAILURE);
                }

                --mem_ptr;
                ++i;
                break;

            case '+':
                ++mem[mem_ptr];
                ++i;
                break;

            case '-':
                --mem[mem_ptr];
                ++i;
                break;

            case '[':
                if (mem[mem_ptr] == 0) {
                    size_t brackets = 1;

                    while (brackets != 0) {
                        ++i;
                        if (program[i] == '[') {
                            ++brackets;
                        } else if (program[i] == ']') {
                            --brackets;
                        }
                    }
                    ++i;
                } else {
                    loops_stack[loops_stack_ptr++] = i;
                    ++i;
                }
                break;

            case ']':
                i = loops_stack[--loops_stack_ptr];
                break;

            case '.':
                putc(mem[mem_ptr], stdout);
                ++i;
                break;

            case ',':
                mem[mem_ptr] = getc(stdin);
                ++i;
                break;
#ifdef DEBUG_ENABLED
            case '@':
                printf("@ debug: ");
                for (size_t j = 0; j <= mem_max; ++j) {
                    if (j == mem_ptr) {
                        printf("*");
                    }

                    printf("(%lu: %d)", j, mem[j]);
                }
                printf("\n");

                ++i;
                break;
#endif
            default:
                break;
        }

        if (mem_ptr > mem_max) {
            mem_max = mem_ptr;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    read_program(argv[1]);
    clean_check_program();
    evaluate_program();
    putc('\n', stdout);

    return 0;
}
