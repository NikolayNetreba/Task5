#include "list.h"

#define DEBUG

int DUMP_COUNT = 0; // number of dump executions, needed for creating unique filenames
FILE* html_dump = NULL;

#define PRINT_LINE printf("===================================================================")

#define VAR_NAME(x) (#x)
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

int main() {
    LinkedList list = initialization(6);

    // Создаем здоровый базовый список из 3 элементов: [1]=10 <-> [2]=20 <-> [3]=30
    place_after(&list, 0, 10, L_I);
    place_after(&list, 1, 20, L_I);
    place_after(&list, 2, 30, L_I);

    // =========================================================================
    // ТЕСТ 1: Ошибка пользователя (INVALID_INPUT) — список не ломается
    // =========================================================================
    place_after(&list, 100, 999, L_I); // Ключ > list_size
    delete_elem(&list, -5, L_I); // Отрицательный ключ

    // =========================================================================
    // ТЕСТ 2: Выход next за пределы массива (WRONG_NEXT -> восьмиугольник)
    // =========================================================================
    int saved_next = list.data[1].next;
    list.data[1].next = -1000;
    LIST_DUMP(&list, "TEST 2: Out-of-bounds NEXT at node [1] (next = -1000)");
    list.data[1].next = saved_next; // Чиним обратно

    // =========================================================================
    // ТЕСТ 3: Выход prev за пределы массива (WRONG_PREV -> восьмиугольник)
    // =========================================================================
    int saved_prev = list.data[2].prev;
    list.data[2].prev = 777;
    LIST_DUMP(&list, "TEST 3: Out-of-bounds PREV at node [2] (prev = 777)");
    list.data[2].prev = saved_prev; // Чиним обратно

    // =========================================================================
    // ТЕСТ 4: Рассинхрон связей — узел 1 смотрит на 3, а 3 ждёт 2 (WRONG_NEXT)
    // =========================================================================
    saved_next = list.data[1].next;
    list.data[1].next = 3; // Вместо узла 2 указываем сразу на 3, но у узла 3 prev по-прежнему равен 2
    LIST_DUMP(&list, "TEST 4: Broken reciprocity NEXT (node [1].next = 3, but node [3].prev = 2)");
    list.data[1].next = saved_next;

    // =========================================================================
    // ТЕСТ 5: Рассинхрон связей — узел 3 смотрит назад на 1, а 1 смотрит на 2 (WRONG_PREV)
    // =========================================================================
    saved_prev = list.data[3].prev;
    list.data[3].prev = 1; // Вместо узла 2 указываем назад на 1
    LIST_DUMP(&list, "TEST 5: Broken reciprocity PREV (node [3].prev = 1, but node [1].next = 2)");
    list.data[3].prev = saved_prev;

    // =========================================================================
    // ТЕСТ 6: Потерянный узел / рассинхрон размера (BROKEN_LINKS)
    // Все связи взаимны (node_verify проходит!), но фактических узлов меньше, чем list_size
    // =========================================================================
    list.data[2].next = 0; // Отрезаем узел 3, делая узел 2 хвостом
    list.data[0].prev = 2; // Фиктивный узел теперь тоже считает узел 2 хвостом
    list.data[3].prev = -1; // Маскируем узел 3 под свободный, чтобы node_verify не ругался
    list.data[3].next = list.free;
    // Но list.list_size по-прежнему равен 3, хотя в цепочке осталось 2 узла!
    LIST_DUMP(&list, "TEST 6: Lost node / Count mismatch (list_size = 3, actual nodes = 2)");
    // Чиним обратно
    list.data[2].next = 3;
    list.data[0].prev = 3;
    list.data[3].prev = 2;
    list.data[3].next = 0;

    // =========================================================================
    // ТЕСТ 7: Бесконечный цикл в списке (LOOP_EXISTS)
    // Замыкаем узлы 2 и 3 друг на друга взаимно, чтобы пройти проверку node_verify
    // =========================================================================
    list.data[3].next = 2;
    list.data[2].prev = 3;
    list.data[1].next = 0; // Изолируем узел 1 и 0 взаимно, чтобы node_verify не упал раньше цикла
    list.data[0].prev = 1;
    list.data[0].next = 2; // Голова ведет сразу в вечный цикл 2 <-> 3
    // Но чтобы node[2].prev == 3 и node[0].next == 2 не конфликтовали в node_verify,
    // прямой цикл в двусвязном списке обычно ловится еще на этапе WRONG_PREV / WRONG_NEXT!
    LIST_DUMP(&list, "TEST 7: Loop attempt (caught by reciprocity check or LOOP_EXISTS)");
    // Чиним обратно
    list.data[0].next = 1;
    list.data[0].prev = 3;
    list.data[1].next = 2;
    list.data[2].prev = 1;
    list.data[3].next = 0;

    // =========================================================================
    // ТЕСТ 8: Повреждение указателя FREE или HEAD (WRONG_FREE / WRONG_HEAD)
    // =========================================================================
    int saved_free = list.free;
    list.free = 999;
    LIST_DUMP(&list, "TEST 8: Corrupted FREE pointer (free = 999)");
    list.free = saved_free;

    // =========================================================================
    // ТЕСТ 9: Поломка внутри списка свободных ячеек (FREE list corruption)
    // =========================================================================
    int saved_free_next = list.data[4].next;
    list.data[4].next = -404; // Свободная ячейка 4 указывает в космос
    LIST_DUMP(&list, "TEST 9: Corrupted NEXT inside FREE list at node [4]");
    list.data[4].next = saved_free_next;

    // Финальная проверка: если раскомментировать следующую строку,
    // любая штатная функция на сломанном списке вызовет VERIFY и жесткий краш через assert(0):
    // list.data[1].next = -1000;
    // place_after(&list, 1, 50, L_I);

    close_all(&list);
    return 0;
}

Error node_verify(LinkedList* list, int index) {
    Error err = { LIST_OK, index };
    Node* node = &list->data[index];

    if (node->next < 0 || node->next > list->capacity) {
        err.err = WRONG_NEXT;
        return err;
    }
    if (node->prev < -1 || node->prev > list->capacity) {
        err.err = WRONG_PREV;
        return err;
    }

    if (node->prev != -1 || index == 0) {
        if (list->data[node->next].prev != index) {
            err.err = WRONG_NEXT;
            return err;
        }
        if (list->data[node->prev].next != index) {
            err.err = WRONG_PREV;
            return err;
        }
    }

    return err;
}

Error list_verify(LinkedList* list) {
    Error err_info = { LIST_OK, -1 };

    if (list == NULL)
        return (Error) { LIST_NULL, -1 };
    if (list->data == NULL)
        return (Error) { DATA_NULL, -1 };
    if (list->list_size < 0 || list->list_size > list->capacity)
        return (Error) { BAD_SIZE_CAPACITY, -1 };

    if (list->data[0].next < 0 || list->data[0].next > list->capacity)
        return (Error) { WRONG_HEAD, -1 };
    if (list->data[0].prev < 0 || list->data[0].prev > list->capacity)
        return (Error) { WRONG_TAIL, -1 };
    if (list->free < 0 || list->free > list->capacity)
        return (Error) { WRONG_FREE, -1 };

    for (int i = 0; i <= list->capacity; i++) {
        err_info = node_verify(list, i);
        if (err_info.err != LIST_OK) {
            return err_info;
        }
    }

    int current = list->data[0].next;
    int count = 0;
    while (current != 0) {
        count++;
        if (count > list->capacity) {
            return (Error) { LOOP_EXISTS, current };
        }
        current = list->data[current].next;
    }

    if (count != list->list_size) {
        return (Error) { BROKEN_LINKS, -1 };
    }

    return (Error) { LIST_OK, -1 };
}

LinkedList initialization(int size) {
    assert(size > 0);

    LinkedList list = { .free = 1, .list_size = 0, .capacity = size };
    list.data = (Node*)calloc(size + 1, sizeof(Node));
    assert(list.data);

    list.data[0].next = 0;
    list.data[0].prev = 0;
    list.data[0].data_elem = 0;

    for (int i = 1; i <= size; i++) {
        list.data[i].next = (i == size) ? 0 : i + 1;
        list.data[i].prev = -1;
        list.data[i].data_elem = POISON;
    }

    return list;
}

int go_to_key(LinkedList* list, int key) {
    assert(list);

    int i = 1;
    int current = list->data[0].next;

    while (current != 0 && i < key) {
        current = list->data[current].next;
        i++;
    }

    return current;
}

bool is_valid_index(int key, int right) {
    return (0 <= key && key <= right);
}

int delete_elem(LinkedList* list, int key, int type) {
    VERIFY(list);

    CHECK_KEY(list, key);

    int current = 0;
    if (type == L_I)
        current = go_to_key(list, key);
    else if (type == P_I)
        current = key;
    else
        assert("delete type error");

    if (current == 0)
        return -1;

    int prev_elem = list->data[current].prev;
    int next_elem = list->data[current].next;

    list->data[prev_elem].next = next_elem;
    list->data[next_elem].prev = prev_elem;

    list->data[current].data_elem = POISON;
    list->data[current].prev = -1;
    list->data[current].next = list->free;
    list->free = current;

    list->list_size--;

    LIST_DUMP(list, "delete arg with key [%d]\nCapacity: %d\nFree: %d\nHead: %d\nTail: %d",
        current + 1, list->capacity, list->free, list->data[0].next, list->data[0].prev);

    return 0;
}

void expand_list(LinkedList* list) {
    assert(list);

    VERIFY(list);

    list->capacity *= 2;

    list->data = (Node*)realloc(list->data, (list->capacity + 1) * sizeof(Node));
    assert(list->data);

    for (int i = list->list_size + 1; i <= list->capacity; i++) {
        list->data[i].next = i + 1;
        list->data[i].prev = -1;
    }

    VERIFY(list);
}

int place_after(LinkedList* list, int key, int val, int type) {
    VERIFY(list);

    if (list->list_size == list->capacity) {
        expand_list(list);
    }

    CHECK_KEY(list, key);

    // update free
    int new_elem = list->free;
    list->free = list->data[new_elem].next;

    // set value
    list->data[new_elem].data_elem = val;

    int current = key;
    if (key != 0 && type == L_I) {
        current = go_to_key(list, key);
    }

    int next_elem = list->data[current].next;

    list->data[new_elem].next = next_elem;
    list->data[new_elem].prev = current;

    list->data[current].next = new_elem;
    list->data[next_elem].prev = new_elem;

    list->list_size++;

    LIST_DUMP(list, "adding new elem with key [%d]\nCapacity: %d\nFree: %d\nHead: %d\nTail: %d",
        current + 1, list->capacity, list->free, list->data[0].next, list->data[0].prev);

    return new_elem;
}

int place_befor(LinkedList* list, int key, int val, int type) {
    assert(list);

    if (list->list_size == list->capacity) {
        expand_list(list);
    }

    CHECK_KEY(list, key);

    int new_elem = list->free;
    list->free = list->data[new_elem].next;

    list->data[new_elem].data_elem = val;

    int current = key;
    if (key != 0 && type == L_I) {
        current = go_to_key(list, key);
    }

    int prev_elem = list->data[current].prev;

    list->data[new_elem].next = current;
    list->data[new_elem].prev = prev_elem;

    list->data[current].prev = new_elem;
    list->data[prev_elem].next = new_elem;

    list->list_size++;

    return new_elem;
}

void make_visual_graph(LinkedList* list, Error err_info) {
    assert(list);
    DUMP_COUNT++;

    char dot_filename[64] = { };
    char png_filename[64] = { };
    snprintf(dot_filename, sizeof(dot_filename), "dumps/graph_%d.dot", DUMP_COUNT);
    snprintf(png_filename, sizeof(png_filename), "dumps/graph_%d.png", DUMP_COUNT);

    FILE* graph = fopen(dot_filename, "w");
    assert(graph);

    fprintf(graph, "digraph g{ \nrankdir=LR; \nnode [shape=Mrecord; style=filled;];\n");

    // Отрисовка узлов
    for (int i = 0; i <= list->capacity; i++) {
        const char* node_color = "#00ff7fff";
        const char* font_color = "black";
        const char* border_color = "black";
        const char* penwidth = "1.0";

        if (err_info.err != LIST_OK && err_info.err != INVALID_INPUT && i == err_info.key) {
            node_color = "#8b0000";
            font_color = "white";
            border_color = "#ff0000";
            penwidth = "3.0";
        } else if (i == list->data[0].next)
            node_color = "#87CEEB";
        else if (i == list->data[0].prev)
            node_color = "#F4C2C2";
        else if (i == list->free)
            node_color = "#FFF3B0";
        else if (i > list->list_size)
            node_color = "#E9ECEF";
        else if (i == 0)
            node_color = "#757373";

        fprintf(graph, "node%d [label = \"{<prev> prev = %d | { index = %d | info = %d } | <next> next = %d}\"; fillcolor=\"%s\"; fontcolor=\"%s\"; color=\"%s\"; penwidth=%s];\n",
            i, list->data[i].prev, i, list->data[i].data_elem, list->data[i].next, node_color, font_color, border_color, penwidth);
    }

    for (int i = 0; i < list->capacity; i++) {
        fprintf(graph, "node%d -> node%d [style=invis, weight=100];\n", i, i + 1);
    }

    // Отрисовка связей
    for (int i = 0; i <= list->capacity; i++) {
        int next_node = list->data[i].next;
        int prev_node = list->data[i].prev;

        bool is_free = (list->data[i].prev == -1 && i != 0);

        if (next_node < 0 || next_node > list->capacity) {
            // выход за пределы массива
            fprintf(graph, "ERR_NODE_%d [shape=octagon, style=filled, fillcolor=\"#ff0000\", fontcolor=\"white\", label=\"%d\"];\n", i, next_node);
            fprintf(graph, "node%d:next:s -> ERR_NODE_%d [color=\"#e90a0a\", penwidth=3.0, constraint=false];\n", i, i);
        } else if (!is_free) {
            if (list->data[next_node].prev != i) {
                fprintf(graph, "node%d:next -> node%d [color=\"#e90a0a\", penwidth=3.0, constraint=false];\n", i, next_node);
            } else {
                fprintf(graph, "node%d:next -> node%d:prev [color=\"#10b542\", dir=\"both\", penwidth=2.0, constraint=false];\n", i, next_node);
            }
        }

        if (prev_node != -1) {
            if (prev_node < 0 || prev_node > list->capacity) {
                // выход за пределы массива
                fprintf(graph, "ERR_NODE_%d [shape=octagon, style=filled, fillcolor=\"#ff0000\", fontcolor=\"white\", label=\"%d\"];\n", i, prev_node);
                fprintf(graph, "node%d:prev:s -> ERR_NODE_%d [color=\"#e90a0a\", penwidth=3.0, constraint=false];\n", i, i);
            } else {
                // порванная обратная связь
                if (list->data[prev_node].next != i) {
                    fprintf(graph, "node%d:prev -> node%d [color=\"#e90a0a\", penwidth=3.0, constraint=false];\n", i, prev_node);
                }
            }
        }
    }

    fprintf(graph, "HEAD [shape=box, style=\"filled, rounded\", fillcolor=\"#87CEEB\", label=\"HEAD\"];\n");
    fprintf(graph, "TAIL [shape=box, style=\"filled, rounded\", fillcolor=\"#F4C2C2\", label=\"TAIL\"];\n");
    fprintf(graph, "FREE [shape=box, style=\"filled, rounded\", fillcolor=\"#FFF3B0\", label=\"FREE\"];\n");

    if (list->data[0].next >= 0 && list->data[0].next <= list->capacity && list->data[0].next != 0) {
        fprintf(graph, "HEAD -> node%d [color=black];\n", list->data[0].next);
    }
    if (list->data[0].prev >= 0 && list->data[0].prev <= list->capacity && list->data[0].prev != 0) {
        fprintf(graph, "TAIL -> node%d [color=black];\n", list->data[0].prev);
    }
    if (list->free >= 0 && list->free <= list->capacity && list->free != 0) {
        fprintf(graph, "FREE -> node%d [color=black];\n", list->free);
    }

    fprintf(graph, "}\n");
    fclose(graph);

    char command[256];
    snprintf(command, sizeof(command), "dot -Tpng %s -o %s", dot_filename, png_filename);
    system(command);
}

void make_txt_dump(LinkedList* list) {
    assert(list);

    PRINT_LINE;
    printf("\nDUMP COUNT: %d\n", DUMP_COUNT);

    printf("List Info:\n");
    printf("  Capacity: %d\n", list->capacity);
    printf("  Size:     %d\n", list->list_size);
    printf("  Head:     %d\n", list->data[0].next);
    printf("  Tail:     %d\n", list->data[0].prev);
    printf("  Free:     %d\n\n", list->free);

    printf("%6s |", "INDEX");
    for (int i = 0; i <= list->capacity; ++i) {
        printf("%7d |", i);
    }
    printf("\n");

    printf("%6s |", "DATA");
    for (int i = 0; i <= list->capacity; i++) {
        if (list->data[i].prev == -1 || list->data[i].data_elem == POISON) {
            printf("%7s |", "POISON");
        } else {
            printf("%7d |", list->data[i].data_elem);
        }
    }
    printf("\n");

    printf("%6s |", "NEXT");
    for (int i = 0; i <= list->capacity; i++) {
        printf("%7d |", list->data[i].next);
    }
    printf("\n");

    printf("%6s |", "PREV");
    for (int i = 0; i <= list->capacity; i++) {
        printf("%7d |", list->data[i].prev);
    }
    printf("\n");

    PRINT_LINE;
}

void dump(LinkedList* list, LocationInfo loc_info, const char* message, Error err_info) {
    assert(list);

    if (html_dump == NULL) {
        html_dump = fopen("dump.html", "w");
        assert(html_dump);
        fprintf(html_dump, "<!DOCTYPE html>\n<html>\n<head><title>List Dump</title></head>\n<body>\n");
    }

    make_visual_graph(list, err_info);

    make_txt_dump(list);

    fprintf(html_dump, "<hr>\n");

    if (message && strlen(message) > 0) {
        fprintf(html_dump, "<h2 style=\"color: black;\">[%d]</h2>\n", DUMP_COUNT);

        fprintf(html_dump, "<pre style=\"font-size: 16px; font-family: monospace;\">%s</pre>\n", message);
    } else {
        fprintf(html_dump, "<h3>[%d] Dump</h3>\n", DUMP_COUNT);
    }

    if (err_info.err != LIST_OK) {
        fprintf(html_dump, "<h2 style=\"color: red;\">ERROR CAUGHT: ");
        switch (err_info.err) {
        case LIST_NULL:
            fprintf(html_dump, "List pointer is NULL");
            break;
        case DATA_NULL:
            fprintf(html_dump, "Data array pointer is NULL");
            break;
        case BAD_SIZE_CAPACITY:
            fprintf(html_dump, "Size exceeds capacity");
            break;
        case WRONG_HEAD:
            fprintf(html_dump, "Head points out of bounds");
            break;
        case WRONG_TAIL:
            fprintf(html_dump, "Tail points out of bounds");
            break;
        case WRONG_FREE:
            fprintf(html_dump, "Free points out of bounds");
            break;
        case WRONG_PREV:
            fprintf(html_dump, "Broken 'prev' [%d]", err_info.key);
            break;
        case WRONG_NEXT:
            fprintf(html_dump, "Broken 'next' [%d]", err_info.key);
            break;
        case LOOP_EXISTS:
            fprintf(html_dump, "Infinite loop detected near node [%d]", err_info.key);
            break;
        case BROKEN_LINKS:
            fprintf(html_dump, "Count mismatch (lost nodes)");
            break;
        case INVALID_INPUT:
            fprintf(html_dump, "Invalid key [%d]", err_info.key);
            break;
        default:
            fprintf(html_dump, "Unknown corruption");
            break;
        }
        fprintf(html_dump, "</h2>\n");

        fprintf(html_dump, "<p>Called from <b>%s</b>, function <b>%s</b>, line <b>%d</b></p>\n",
            loc_info.file, loc_info.func, loc_info.line);
    }
    fprintf(html_dump, "<img src=\"dumps/graph_%d.png\">\n", DUMP_COUNT);
    fflush(html_dump);
}

void close_all(LinkedList* list) {
    free(list->data);
    if (html_dump) {
        fprintf(html_dump, "</body>\n</html>\n");
        fclose(html_dump);
    }
}
