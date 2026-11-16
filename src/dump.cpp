#include "list.h"

int DUMP_COUNT = 0;
FILE* html_dump = NULL;

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

    for (int i = 0; i <= list->capacity; i++) {
        int next_node = list->data[i].next;
        int prev_node = list->data[i].prev;
        bool is_free = (list->data[i].prev == -1 && i != 0);

        if (next_node < 0 || next_node > list->capacity) {
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
                fprintf(graph, "ERR_NODE_%d [shape=octagon, style=filled, fillcolor=\"#ff0000\", fontcolor=\"white\", label=\"%d\"];\n", i, prev_node);
                fprintf(graph, "node%d:prev:s -> ERR_NODE_%d [color=\"#e90a0a\", penwidth=3.0, constraint=false];\n", i, i);
            } else {
                if (list->data[prev_node].next != i) {
                    fprintf(graph, "node%d:prev -> node%d [color=\"#e90a0a\", penwidth=3.0, constraint=false];\n", i, prev_node);
                }
            }
        }
    }

    fprintf(graph, "HEAD [shape=box, style=\"filled, rounded\", fillcolor=\"#87CEEB\", label=\"HEAD\"];\n");
    fprintf(graph, "TAIL [shape=box, style=\"filled, rounded\", fillcolor=\"#F4C2C2\", label=\"TAIL\"];\n");
    fprintf(graph, "FREE [shape=box, style=\"filled, rounded\", fillcolor=\"#FFF3B0\", label=\"FREE\"];\n");

    if (list->data[0].next >= 0 && list->data[0].next <= list->capacity && list->data[0].next != 0)
        fprintf(graph, "HEAD -> node%d [color=black];\n", list->data[0].next);
    if (list->data[0].prev >= 0 && list->data[0].prev <= list->capacity && list->data[0].prev != 0)
        fprintf(graph, "TAIL -> node%d [color=black];\n", list->data[0].prev);
    if (list->free >= 0 && list->free <= list->capacity && list->free != 0)
        fprintf(graph, "FREE -> node%d [color=black];\n", list->free);

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

    printf("List Info:\n  Capacity: %d\n  Size:     %d\n  Head:     %d\n  Tail:     %d\n  Free:     %d\n\n",
        list->capacity, list->list_size, list->data[0].next, list->data[0].prev, list->free);

    printf("%6s |", "INDEX");
    for (int i = 0; i <= list->capacity; ++i)
        printf("%7d |", i);
    printf("\n%6s |", "DATA");
    for (int i = 0; i <= list->capacity; i++) {
        if (list->data[i].prev == -1 || list->data[i].data_elem == POISON)
            printf("%7s |", "POISON");
        else
            printf("%7d |", list->data[i].data_elem);
    }
    printf("\n%6s |", "NEXT");
    for (int i = 0; i <= list->capacity; i++)
        printf("%7d |", list->data[i].next);
    printf("\n%6s |", "PREV");
    for (int i = 0; i <= list->capacity; i++)
        printf("%7d |", list->data[i].prev);
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
        fprintf(html_dump, "<pre style=\"font-size: 16px;\">%s</pre>\n", message);
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
