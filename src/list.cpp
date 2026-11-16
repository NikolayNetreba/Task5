#include "list.h"

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
        if (err_info.err != LIST_OK)
            return err_info;
    }

    int current = list->data[0].next;
    int count = 0;
    while (current != 0) {
        count++;
        if (count > list->capacity)
            return (Error) { LOOP_EXISTS, current };
        current = list->data[current].next;
    }

    if (count != list->list_size)
        return (Error) { BROKEN_LINKS, -1 };

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
    if (list->list_size == list->capacity)
        expand_list(list);
    CHECK_KEY(list, key);

    int new_elem = list->free;
    list->free = list->data[new_elem].next;
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
    if (list->list_size == list->capacity)
        expand_list(list);
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

void close_all(LinkedList* list) {
    free(list->data);
    if (html_dump) {
        fprintf(html_dump, "</body>\n</html>\n");
        fclose(html_dump);
    }
}
