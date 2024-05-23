#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table, ValueType keyType) {
    table->count = 0;
    table->capacity = 0;
    table->keyType = keyType;
    table->entries = NULL;
}

void freeTable(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table, table->keyType);
}

static Entry *findEntry(Entry *entries, int capacity, Value key, ValueType keyType) {
    VALUE_HASH(key, keyType);
    uint32_t index = valueHash % capacity;
    Entry *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];
        if (IS_NIL(entry->key)) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (valuesEqual(entry->key, key)) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjustCapacity(Table *table, int capacity) {
    Entry *entries = ALLOCATE(Entry, capacity);
    table->count = 0;
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NIL_VAL;
        entries[i].value = NIL_VAL;
    }

    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (IS_NIL(entry->key)) continue;

        Entry *dest = findEntry(entries, capacity, entry->key, table->keyType);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableGet(Table *table, Value key, Value *value) {
    if (table->count == 0) return false;

    Entry *entry = findEntry(table->entries, table->capacity, key, table->keyType);
    if (IS_NIL(entry->key)) return false;

    *value = entry->value;
    return true;
}

bool tableSet(Table *table, Value key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key, table->keyType);
    bool isNewKey = IS_NIL(entry->key);
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table *table, Value *key) {
    if (table->count == 0) return false;

    Entry *entry = findEntry(table->entries, table->capacity, *key, table->keyType);
    if (IS_NIL(entry->key)) return false;

    entry->key = NIL_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll(Table *from, Table *to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];
        if (IS_NIL(entry->key)) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }
    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[index];
        if (IS_NIL(entry->key)) {
            if (IS_NIL(entry->value)) return NULL;
        } else {
            ObjString *string = AS_STRING(entry->key);
            if (string->length == length && string->hash == hash && memcmp(AS_CSTRING(string), chars, length) == 0) {
                return string;
            }
        }

        index = (index + 1) % table->capacity;
    }
}