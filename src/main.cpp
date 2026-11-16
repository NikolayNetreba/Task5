#include "list.h"

int main() {
    LinkedList list = initialization(6);

    place_after(&list, 0, 10, L_I);
    place_after(&list, 1, 20, L_I);
    place_after(&list, 2, 30, L_I);

    // =========================================================================
    // ТЕСТ 1: Ошибка пользователя
    place_after(&list, 100, 999, L_I); // Ключ > list_size
    delete_elem(&list, -5, L_I); // Отрицательный ключ

    // =========================================================================
    // ТЕСТ 2: Выход next за пределы массива
    int saved_next = list.data[1].next;
    list.data[1].next = -1000;
    LIST_DUMP(&list, "TEST 2: NEXT at node [1] (next = -1000)");
    list.data[1].next = saved_next;

    // =========================================================================
    // ТЕСТ 3: Выход prev за пределы массива
    int saved_prev = list.data[2].prev;
    list.data[2].prev = 777;
    LIST_DUMP(&list, "TEST 3: PREV at node [2] (prev = 777)");
    list.data[2].prev = saved_prev;

    // =========================================================================
    // ТЕСТ 4: Ломаем связи между вершинами (next)
    saved_next = list.data[1].next;
    list.data[1].next = 3;
    LIST_DUMP(&list, "TEST 4: (node [1].next = 3, but node [3].prev = 2)");
    list.data[1].next = saved_next;

    // =========================================================================
    // ТЕСТ 5: Ломаем связи между вершинами (prev)
    saved_prev = list.data[3].prev;
    list.data[3].prev = 1;
    LIST_DUMP(&list, "TEST 5: (node [3].prev = 1, but node [1].next = 2)");
    list.data[3].prev = saved_prev;

    // =========================================================================
    // ТЕСТ 6: Ломаем FREE или HEAD
    int saved_free = list.free;
    list.free = 999;
    LIST_DUMP(&list, "TEST 6: Corrupted FREE pointer (free = 999)");
    list.free = saved_free;

    // =========================================================================
    // ТЕСТ 7: Ломаем свободные ячейки
    int saved_free_next = list.data[4].next;
    list.data[4].next = -404;
    LIST_DUMP(&list, "TEST 7: Corrupted NEXT inside FREE list at node [4]");
    list.data[4].next = saved_free_next;

    close_all(&list);
    return 0;
}
