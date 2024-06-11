#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MEM_SIZE              500  // 500 bytes of memory
#define MAX_STACK_SIZE        200  // 200 bytes of memory for the stack
#define MAX_HEAP_SIZE         300  // 300 bytes of memory for the heap
#define FRAME_STATUS_SIZE     21   // 21 bytes of memory for the frame status
#define MAX_FRAMES            5    // 5 frames
#define MIN_FRAME_SIZE        10   // 10 bytes of memory for each frame
#define MAX_FRAME_SIZE        80   // 80 bytes of memory for each frame
#define MAX_NAME_SIZE         8    // 8 bytes of memory for each name
#define MAX_INT               20   // 20 integers per frame
#define MAX_DOUBLE            10   // 10 doubles per frame
#define MAX_CHAR              80   // 80 chars per frame
#define MAX_POINTER           20   // 20 pointers per frame
#define FRAME_METADATA_OFFSET sizeof(frame_status_t)
#define BUFFER_METADATA_SIZE  sizeof(allocated_t)

/**
 * @brief Structure to store the frame status
 *
 * @details This structure is used to store the status of the frame, whether it is used or not, the
 *         function address, frame address and the name of the function.
 *
 */
typedef struct __frame_status_t {
    int  number;
    char name[MAX_NAME_SIZE];
    int  func_address;
    int  frame_address;
    bool used;
} frame_status_t;

/**
 * @brief Structure to store the free list
 *
 * @details This structure is used to store the free list, it stores the start address of the free
 *        block, the size of the free block and a pointer to the next free block.
 *
 */
typedef struct __freelist_t {
    int                  start;
    int                  size;
    struct __freelist_t *next;
} freelist_t;

/**
 * @brief Structure to store the allocated buffer
 *
 * @details This structure is used to store the allocated buffer, it stores the name of the buffer,
 *         the start address of the buffer and the size of the buffer.
 *
 */
typedef struct __allocated_t {
    char name[MAX_NAME_SIZE];
    int  start_address;
} allocated_t;

/**
 * @brief Structure to store the integer
 *
 * @details This structure is used to store the integer, it stores the name of the integer, the
 *        value of the integer and a flag to check if the integer has been initialized or not.
 *
 */
typedef struct __my_int_t {
    char name[MAX_NAME_SIZE];
    int  value;
    bool initialized;
} int_t;

/**
 * @brief Structure to store the double
 *
 * @details This structure is used to store the double, it stores the name of the double, the
 *        value of the double and a flag to check if the double has been initialized or not.
 *
 */
typedef struct __my_double_t {
    char   name[MAX_NAME_SIZE];
    double value;
    bool   initialized;
} double_t;

/**
 * @brief Structure to store the char
 *
 * @details This structure is used to store the char, it stores the name of the char, the
 *        value of the char and a flag to check if the char has been initialized or not.
 *
 */
typedef struct __my_char_t {
    char name[MAX_NAME_SIZE];
    char value;
    bool initialized;
} char_t;

/**
 * @brief Structure to store the frame
 *
 * @details This structure is used to store the frame, it stores the frame address, the size of the
 *       frame, the integer, double, char and pointer arrays.
 *
 */
typedef struct __frame_t {
    int                  frame_address;
    int                  size;
    struct __my_int_t    my_ints[MAX_INT];
    struct __my_double_t my_doubles[MAX_DOUBLE];
    struct __my_char_t   my_chars[MAX_CHAR];
    void                *pointers[MAX_POINTER];
} frame_t;

/**
 * @brief Structure to store the memory
 *
 * @details This structure is used to store the memory, it stores the frame status, stack frame,
 *       free list, stack size, heap size and the heap.
 *
 */
typedef struct __memory_t {
    struct __frame_status_t frame_status[MAX_FRAMES];
    struct __frame_t        stack_frame[MAX_FRAMES];
    struct __freelist_t    *freelist_head;
    int                     stack_size;
    int                     heap_size;
    char                    heap[MAX_HEAP_SIZE];
} memory_t;

memory_t sys_memory;  // Global variable to store the memory

/**
 * @brief Function to initialize the memory
 *
 * @details This function is used to initialize the memory, it initializes the frame status, stack
 *      frame, free list, stack size, heap size and the heap. It also initializes the free list
 *     head.
 *
 */
void init() {
    for (int i = 0; i < MAX_FRAMES; ++i) {
        sys_memory.frame_status[i] = (frame_status_t){
            .number        = 0,
            .func_address  = 0,
            .frame_address = 0,
            .used          = false,
        };
        memset(sys_memory.frame_status[i].name, '\0', sizeof(sys_memory.frame_status[i].name));

        sys_memory.stack_frame[i] = (frame_t){
            .frame_address = -1,
            .size          = 0,
        };
        memset(sys_memory.stack_frame[i].my_ints, 0, sizeof(sys_memory.stack_frame[i].my_ints));
        memset(sys_memory.stack_frame[i].my_doubles, 0, sizeof(sys_memory.stack_frame[i].my_doubles));
        memset(sys_memory.stack_frame[i].my_chars, 0, sizeof(sys_memory.stack_frame[i].my_chars));
        memset(sys_memory.stack_frame[i].pointers, 0, sizeof(sys_memory.stack_frame[i].pointers));
    }

    sys_memory.freelist_head = (freelist_t *)malloc(sizeof(freelist_t));
    if (!sys_memory.freelist_head) {
        fprintf(stderr, "Error: Could not allocate memory for the heap free list\n");
        exit(EXIT_FAILURE);
    }

    sys_memory.stack_size = 0;
    sys_memory.heap_size  = 0;

    sys_memory.freelist_head = &(freelist_t){
        .start = 0,
        .size  = MAX_HEAP_SIZE,
        .next  = NULL,
    };
}

/**
 * @brief Function to create a new frame
 *
 * @details This function is used to create a new frame, it checks if the function name is valid,
 *      if the stack size is less than the maximum stack size and if the function already exists.
 *
 * @param func_name
 * @param func_address
 */
void CF(char *func_name, int func_address) {
    if (strlen(func_name) > MAX_NAME_SIZE) {
        fprintf(stderr, "Error: Function name too long, name can be of at most 8 characters.\n");
        return;
    } else if (sys_memory.stack_size + FRAME_METADATA_OFFSET > MAX_STACK_SIZE) {
        fprintf(stderr, "Error: Stack overflow, not enough memory available for new function\n");
        return;
    }

    for (int i = 0; i < MAX_FRAMES; ++i) {
        if (sys_memory.frame_status[i].used && strcmp(sys_memory.frame_status[i].name, func_name) == 0) {
            fprintf(stderr, "Error: Function already exists\n");
            return;
        }
    }

    for (int i = 0; i < MAX_FRAMES; ++i) {
        if (!sys_memory.frame_status[i].used) {
            sys_memory.frame_status[i] =
                (frame_status_t){.used          = true,
                                 .number        = i + 1,
                                 .func_address  = func_address,
                                 .frame_address = MEM_SIZE - sys_memory.stack_size - FRAME_METADATA_OFFSET};
            strncpy(sys_memory.frame_status[i].name, func_name, sizeof(sys_memory.frame_status[i].name) - 1);

            sys_memory.stack_frame[i].frame_address = sys_memory.frame_status[i].frame_address;

            sys_memory.stack_size += FRAME_METADATA_OFFSET;

            return;
        }
    }

    printf("Error: Cannot create another frame, maximum number of frames have been reached\n");
    return;
}

/**
 * @brief Function to delete a frame
 *
 * @details This function is used to delete a frame, it checks if the stack is empty, if the frame
 *     exists and then deletes the frame.
 *
 */
void DF() {
    if (sys_memory.stack_size == 0) {
        fprintf(stderr, "Error: Stack is empty, no functions to delete\n");
        return;
    }

    for (int i = MAX_FRAMES - 1; i >= 0; --i) {
        if (sys_memory.frame_status[i].used) {
            sys_memory.frame_status[i] =
                (frame_status_t){.used = false, .number = 0, .func_address = -1, .frame_address = -1};
            memset(sys_memory.frame_status[i].name, '\0', sizeof(sys_memory.frame_status[i].name));

            sys_memory.stack_size -= sys_memory.stack_frame[i].size;

            memset(&sys_memory.stack_frame[i], 0, sizeof(sys_memory.stack_frame[i]));

            return;
        }
    }
}

/**
 * @brief Function to create an integer
 *
 * @param name
 * @param value
 */
void CI(char *name, int value) {
    int curr_frame = -1;
    for (int i = MAX_FRAMES - 1; i >= 0; --i) {
        if (sys_memory.frame_status[i].used) {
            curr_frame = i;
            break;
        }
    }

    if (curr_frame == -1) {
        fprintf(stderr, "Error: No frames exist, cannot create integer\n");
        return;
    } else if (sys_memory.stack_frame[curr_frame].size + sizeof(int) > MAX_FRAME_SIZE) {
        fprintf(stderr, "Error: The frame is full, cannot create more data on it\n");
        return;
    }

    for (int i = 0; i < MAX_INT; ++i) {
        if (!sys_memory.stack_frame[curr_frame].my_ints[i].initialized) {
            strncpy(sys_memory.stack_frame[curr_frame].my_ints[i].name, name,
                    sizeof(sys_memory.stack_frame[curr_frame].my_ints[i].name));
            sys_memory.stack_frame[curr_frame].my_ints[i].value       = value;
            sys_memory.stack_frame[curr_frame].my_ints[i].initialized = true;
            sys_memory.stack_frame[curr_frame].size += sizeof(int);
            sys_memory.stack_size += sizeof(int);

            return;
        }
    }
}

/**
 * @brief Function to create a double
 *
 * @param name
 * @param value
 */
void CD(char *name, double value) {
    int curr_frame = -1;
    for (int i = MAX_FRAMES - 1; i >= 0; --i) {
        if (sys_memory.frame_status[i].used) {
            curr_frame = i;
            break;
        }
    }

    if (curr_frame == -1) {
        fprintf(stderr, "Error: No frames exist, cannot create double\n");
        return;
    } else if (sys_memory.stack_frame[curr_frame].size + sizeof(double) > MAX_FRAME_SIZE) {
        fprintf(stderr, "Error: The frame is full, cannot create more data on it\n");
        return;
    }

    for (int i = 0; i < MAX_DOUBLE; ++i) {
        if (!sys_memory.stack_frame[curr_frame].my_doubles[i].initialized) {
            strncpy(sys_memory.stack_frame[curr_frame].my_doubles[i].name, name,
                    sizeof(sys_memory.stack_frame[curr_frame].my_doubles[i].name));
            sys_memory.stack_frame[curr_frame].my_doubles[i].value       = value;
            sys_memory.stack_frame[curr_frame].my_doubles[i].initialized = true;
            sys_memory.stack_frame[curr_frame].size += sizeof(double);
            sys_memory.stack_size += sizeof(double);

            return;
        }
    }
}

/**
 * @brief Function to create a char
 *
 * @param name
 * @param value
 */
void CC(char *name, char value) {
    int curr_frame = -1;
    for (int i = MAX_FRAMES - 1; i >= 0; --i) {
        if (sys_memory.frame_status[i].used) {
            curr_frame = i;
            break;
        }
    }

    if (curr_frame == -1) {
        fprintf(stderr, "Error: No frames exist, cannot create char\n");
        return;
    } else if (sys_memory.stack_frame[curr_frame].size + sizeof(char) > MAX_FRAME_SIZE) {
        fprintf(stderr, "Error: The frame is full, cannot create more data on it\n");
        return;
    }
    for (int i = 0; i < MAX_CHAR; ++i) {
        if (!sys_memory.stack_frame[curr_frame].my_chars[i].initialized) {
            strncpy(sys_memory.stack_frame[curr_frame].my_chars[i].name, name,
                    sizeof(sys_memory.stack_frame[curr_frame].my_chars[i].name));
            sys_memory.stack_frame[curr_frame].my_chars[i].value       = value;
            sys_memory.stack_frame[curr_frame].my_chars[i].initialized = true;
            sys_memory.stack_frame[curr_frame].size += sizeof(char);
            sys_memory.stack_size += sizeof(char);

            return;
        }
    }
}

/**
 * @brief Function to create a heap
 *
 * @param buffer_name
 * @param size
 */
void CH(char *buffer_name, int size) {
    if (sys_memory.heap_size + size + BUFFER_METADATA_SIZE > MAX_HEAP_SIZE) {
        fprintf(stderr, "Error: The heap is full, cannot create more data\n");
        return;
    }

    int frame_idx   = -1;
    int pointer_idx = -1;
    int i           = MAX_FRAMES - 1;
    while (i >= 0) {
        if (sys_memory.frame_status[i].used == false) {
            --i;
        } else {
            frame_idx = i;
            for (int j = 0; j < MAX_POINTER; ++j) {
                if (sys_memory.stack_frame[i].pointers[j] == NULL) {
                    pointer_idx = j;
                    break;
                }
            }
            if (pointer_idx != -1) {
                break;
            }
            --i;
        }
    }

    if (pointer_idx == -1) {
        fprintf(stderr, "Error: No pointers available in frame, cannot create buffer\n");
        return;
    } else if (frame_idx == -1) {
        fprintf(stderr, "Error: No frames exist, cannot create buffer\n");
        return;
    }

    allocated_t *buffer_meta   = (allocated_t *)(sys_memory.heap + sys_memory.heap_size);
    buffer_meta->start_address = sys_memory.heap_size + BUFFER_METADATA_SIZE;
    strncpy(buffer_meta->name, buffer_name, 7);
    buffer_meta->name[7] = '\0';

    sys_memory.heap_size += size + BUFFER_METADATA_SIZE;
    sys_memory.stack_frame[frame_idx].pointers[pointer_idx] = (void *)&sys_memory.heap[buffer_meta->start_address];

    return;
}

/**
 * @brief Function to print the stack and heap
 *
 */
void SM() {
    printf("                               STACK\n");
    printf("|-------|---------------|------------------|---------------|------------|\n");
    printf("| Frame | Function Name | Function Address | Frame Address | Frame Size |\n");
    printf("|-------|---------------|------------------|---------------|------------|\n");
    for (int i = MAX_FRAMES - 1; i >= 0; --i) {
        if (sys_memory.frame_status[i].used) {
            printf("| %-5d | %-13s | 0x%-14X | %-13d | %-10d |\n", sys_memory.frame_status[i].number,
                   sys_memory.frame_status[i].name, sys_memory.frame_status[i].func_address,
                   sys_memory.frame_status[i].frame_address, sys_memory.stack_frame[i].size);
        }
    }
    printf("|-------|---------------|------------------|---------------|------------|\n");

    for (int i = MAX_FRAMES - 1; i >= 0; --i) {
        if (sys_memory.frame_status[i].used) {
            printf("\n\nFrame %d Contents:\n", sys_memory.frame_status[i].number);
            printf("|---------------|----------|-----------------|\n");
            printf("| Variable Name |   Type   |      Value      |\n");
            printf("|---------------|----------|-----------------|\n");
            for (int j = 0; j < MAX_INT; ++j) {
                if (sys_memory.stack_frame[i].my_ints[j].initialized) {
                    printf("| %-13s | int      | %-15d |\n", sys_memory.stack_frame[i].my_ints[j].name,
                           sys_memory.stack_frame[i].my_ints[j].value);
                }
            }
            for (int j = 0; j < MAX_DOUBLE; ++j) {
                if (sys_memory.stack_frame[i].my_doubles[j].initialized) {
                    printf("| %-13s | double   | %-15lf |\n", sys_memory.stack_frame[i].my_doubles[j].name,
                           sys_memory.stack_frame[i].my_doubles[j].value);
                }
            }
            for (int j = 0; j < MAX_CHAR; ++j) {
                if (sys_memory.stack_frame[i].my_chars[j].initialized) {
                    printf("| %-13s | char     | %-15c |\n", sys_memory.stack_frame[i].my_chars[j].name,
                           sys_memory.stack_frame[i].my_chars[j].value);
                }
            }
            for (int j = 0; j < MAX_POINTER; ++j) {
                if (sys_memory.stack_frame[i].pointers[j] != NULL) {
                    printf("| %-13s | pointer  | %-15p |\n", "pointer", sys_memory.stack_frame[i].pointers[j]);
                }
            }
            printf("|---------------|----------|-----------------|\n");
        }
    }

    int curr_addr = 0;
    printf("\nHEAP\n");
    printf("Heap Size: %d\n", sys_memory.heap_size);
    printf("|---------------|-----------------|--------|\n");
    printf("|  Buffer Name  |  Start Address  |  Size  |\n");
    printf("|---------------|-----------------|--------|\n");
    while (curr_addr < sys_memory.heap_size) {
        allocated_t *buffer_meta = (allocated_t *)(sys_memory.heap + curr_addr);
        int          bufferSize  = *(int *)(sys_memory.heap + curr_addr + BUFFER_METADATA_SIZE - sizeof(int));
        printf("| %-13s | 0x%-13d | %-6d |\n", buffer_meta->name, buffer_meta->start_address, bufferSize);
        curr_addr += bufferSize + BUFFER_METADATA_SIZE;
    }

    printf("|---------------|-----------------|--------|\n\n");
}

/**
 * @brief Main function
 *
 * @return int
 */
int main() {
    char   input[3], name[8];
    int    func_address, int_value, buffer_size;
    double double_value;
    char   char_value;

    printf("Type Q or q to quit\n");
    init();
    while (1) {
        printf("$ ");
        scanf("%s", input);
        if (strcmp(input, "Q") == 0 || strcmp(input, "q") == 0) {
            exit(EXIT_SUCCESS);
        } else if (strcmp(input, "DF") == 0) {
            DF();
        } else if (strcmp(input, "SM") == 0) {
            SM();
        } else if (strcmp(input, "CF") == 0) {
            scanf("%s %d", name, &func_address);
            CF(name, func_address);
        } else if (strcmp(input, "CI") == 0) {
            scanf("%s %d", name, &int_value);
            CI(name, int_value);
        } else if (strcmp(input, "CD") == 0) {
            scanf("%s %lf", name, &double_value);
            CD(name, double_value);
        } else if (strcmp(input, "CC") == 0) {
            scanf("%s %c", name, &char_value);
            CC(name, char_value);
        } else if (strcmp(input, "CH") == 0) {
            scanf("%s %d", name, &buffer_size);
            CH(name, buffer_size);
        } else if (strcmp(input, "DH") == 0) {
            scanf("%s", name);
            printf("Not Implemented yet!\n");
        } else {
            printf("Invalid input, please try again\n");
        }
    }

    return 0;
}
