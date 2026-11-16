#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG
#define POISON 0xDED
#define PRINT_LINE printf("===================================================================")
#define VAR_NAME(x) (#x)

enum ErrorCode {
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
};

enum InsertType {
    L_I, // Logical index
    P_I // Physical index
};

typedef struct {
    int err;
    int key;
} Error;

typedef struct {
    char* file;
    char* func;
    int line;
    char* name;
} LocationInfo;

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

extern int DUMP_COUNT;
extern FILE* html_dump;

LinkedList initialization(int size);
int go_to_key(LinkedList* list, int key);
bool is_valid_index(int key, int right);
int delete_elem(LinkedList* list, int key, int type);
void expand_list(LinkedList* list);
int place_after(LinkedList* list, int key, int val, int type);
int place_befor(LinkedList* list, int key, int val, int type);
Error node_verify(LinkedList* list, int index);
Error list_verify(LinkedList* list);
void close_all(LinkedList* list);

void make_visual_graph(LinkedList* list, Error err_info);
void make_txt_dump(LinkedList* list);
void dump(LinkedList* list, LocationInfo loc_info, const char* message, Error err_info);

#ifdef DEBUG
#define GET_LOCATION_INFO(source_locate, var)  \
    source_locate.file = strdup(__FILE__);     \
    source_locate.func = strdup(__FUNCTION__); \
    source_locate.line = __LINE__;             \
    source_locate.name = strdup(VAR_NAME(var));

#define LIST_DUMP(list_ptr, ...)                         \
    do {                                                 \
        char message[256] = { };                         \
        snprintf(message, sizeof(message), __VA_ARGS__); \
        Error err = list_verify(list_ptr);               \
        LocationInfo loc = { };                          \
        GET_LOCATION_INFO(loc, *list_ptr)                \
        dump(list_ptr, loc, message, err);               \
        if (err.err != LIST_OK) {                        \
            assert(1 && "List got corrupted");           \
        }                                                \
    } while (0);

#define VERIFY(list_ptr)                       \
    do {                                       \
        Error err = list_verify(list_ptr);     \
        LocationInfo loc = { };                \
        GET_LOCATION_INFO(loc, *list_ptr);     \
        if (err.err != LIST_OK) {              \
            dump(list_ptr, loc, " ", err);     \
            assert(1 && "List got corrupted"); \
        }                                      \
    } while (0);

#define CHECK_KEY(list_ptr, key_val)                                                      \
    do {                                                                                  \
        if (is_valid_index((key_val), (list_ptr)->list_size) == 0) {                      \
            LocationInfo loc = { };                                                       \
            GET_LOCATION_INFO(loc, *(list_ptr));                                          \
            Error safe_err = { INVALID_INPUT, (key_val) };                                \
            char msg[128];                                                                \
            snprintf(msg, sizeof(msg),                                                    \
                "WARNING: Ignored operation. Invalid key [%d] passed, list_size is [%d]", \
                (key_val), (list_ptr)->list_size);                                        \
            dump((list_ptr), loc, msg, safe_err);                                         \
            return -1;                                                                    \
        }                                                                                 \
    } while (0)
#else
#define GET_LOCATION_INFO(source_locate, var)
#define LIST_DUMP(list_ptr, ...)
#define VERIFY(list_ptr)
#define CHECK_KEY(list_ptr, key_val)                                 \
    do {                                                             \
        if (is_valid_index((key_val), (list_ptr)->list_size) == 0) { \
            return -1;                                               \
        }                                                            \
    } while (0)
#endif
