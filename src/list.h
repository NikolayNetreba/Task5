#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int data_elem;
    int next;
    int prev;
} Node;

typedef struct {
    Node* data;
    int free;
    int list_size;
    int capacity;
} LinkedList;

#define P_I 1 // PHYSICAL_INDX
#define L_I 0 // LOGICAL_INDX

#define POISON -1

typedef struct {
    char* file;
    char* func;
    int line;
    char* name;
} LocationInfo;

typedef enum {
    LIST_OK = 0,
    LIST_NULL,
    DATA_NULL,
    BAD_SIZE_CAPACITY,
    WRONG_HEAD,
    WRONG_TAIL,
    WRONG_FREE,
    WRONG_PREV,
    WRONG_NEXT,
    LOOP_EXISTS,
    BROKEN_LINKS,
    INVALID_INPUT
} Error_t;

typedef struct {
    Error_t err;
    int key;

} Error;

Error list_verify(LinkedList* list);
LinkedList initialization(int size);
int go_to_key(LinkedList* list, int key);
bool is_valid_index(int key, int right);
int delete_elem(LinkedList* list, int key, int type);
void expand_list(LinkedList* list);
int place_after(LinkedList* list, int key, int val, int type);
void make_visual_graph(LinkedList* list, Error err_info);
void dump(LinkedList* list, LocationInfo loc_info, const char* message, Error err);
void close_all(LinkedList* list);
