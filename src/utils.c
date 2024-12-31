#include "feeny/utils.h"
#include "feeny/collector.h"

//============================================================
//===================== CONVENIENCE ==========================
//============================================================

int max(int a, int b) {
    return a > b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

void print_string(char *str) {
    printf("\"");
    while (1) {
        char c = str[0];
        str++;
        switch (c) {
        case '\n':
            printf("\\n");
            break;
        case '\\':
            printf("\\\\");
            break;
        case '"':
            printf("\\\"");
            break;
        case 0:
            printf("\"");
            return;
        default:
            printf("%c", c);
            break;
        }
    }
}

//============================================================
//===================== VECTORS ==============================
//============================================================

#include <execinfo.h>
#include <stdio.h>

void print_trace(const char *func_name) {
    void *array[10];
    int size = backtrace(array, 10);
    char **strings = backtrace_symbols(array, size);

    printf("\n=== %s called from: ===\n", func_name);
    for (int i = 0; i < size; i++) {
        printf("%s\n", strings[i]);
    }
    printf("=====================\n");

    free(strings);
}

Vector *make_vector() {
    // print_trace(__func__);
    Vector *v = (Vector *)malloc(sizeof(Vector));
    v->size = 0;
    v->capacity = 8;
    v->array = malloc(sizeof(void *) * v->capacity);
    return v;
}

void vector_ensure_capacity(Vector *v, int c) {
    if (v->capacity >= c) {
        return;
    }
    int new_capacity;
    if (v->capacity < 1024) {
        new_capacity = max(v->capacity * 2, c);
    } else {
        new_capacity = max(v->capacity + (v->capacity >> 1), c);
    }

    void **a2 = malloc(sizeof(void *) * new_capacity);
    memcpy(a2, v->array, sizeof(void *) * v->size);
    free(v->array);
    v->capacity = new_capacity;
    v->array = a2;
}

void vector_set_length(Vector *v, int len, void *x) {
    if (len < 0) {
        printf("Negative length given to vector.\n");
        exit(-1);
    }
    if (len <= v->size) {
        v->size = len;
    } else {
        while (v->size < len)
            vector_add(v, x);
    }
}

void vector_add(Vector *v, void *val) {
    vector_ensure_capacity(v, v->size + 1);
    v->array[v->size] = val;
    v->size++;
}

void *vector_pop(Vector *v) {
    if (v->size == 0) {
        printf("Pop from empty vector.\n");
        exit(-1);
    }
    v->size--;
    return v->array[v->size];
}

void *vector_peek(Vector *v) {
    if (v->size == 0) {
        printf("Peek from empty vector.\n");
        exit(-1);
    }
    return v->array[v->size - 1];
}

void vector_clear(Vector *v) {
    v->size = 0;
}

void vector_free(Vector *v) {
    free(v->array);
    free(v);
}

void *vector_get(Vector *v, int i) {
    if (i < 0 || i >= v->size) {
        printf("Index %d out of bounds.\n", i);
        exit(-1);
    }
    return v->array[i];
}

void vector_set(Vector *v, int i, void *x) {
    if (i < 0 || i > v->size) {
        printf("Index %d out of bounds.\n", i);
        exit(-1);
    } else if (i == v->size) {
        vector_add(v, x);
    } else {
        v->array[i] = x;
    }
}

int vector_size(Vector *v) {
    return v->size;
}

//============================================================
//========================= MAP ==============================
//============================================================

Map *newMap() {
    Map *new = (Map *)malloc(sizeof(Map));
    new->names = make_vector();
    new->values = make_vector();
    return new;
}

void freeMap(Map *mp) {
    vector_free(mp->names);
    vector_free(mp->values);
    free(mp);
}

void *findByName(Map *map, char *name) {
    int pos = -1;
    for (int i = 0; i < vector_size(map->names); i++) {
        if (strcmp(name, (char *)vector_get(map->names, i)) == 0) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return NULL;
    } else {
        return vector_get(map->values, pos);
    }
}

void addNewTuple(Map *map, char *name, void *value) {
    int pos = -1;
    for (int i = 0; i < vector_size(map->names); i++) {
        if (strcmp(name, (char *)vector_get(map->names, i)) == 0) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        vector_add(map->names, name);
        vector_add(map->values, value);
    } else {
        vector_set(map->values, pos, value);
    }
}