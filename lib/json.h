#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

// JSON structures
struct json_object {
    char *key;
    char *value_string;
    double value_number;
    bool value_boolean;
    bool value_null;
    bool is_number;
    bool is_string;
    bool is_boolean;
    bool is_null;
    struct json_object *next;
};

struct json_array {
    struct json_object *objects;
    struct json_array *next;
};

struct json_root {
    struct json_object *object;
    struct json_array *array;
    bool is_array;
};

// Function declarations
struct json_root* parse_json(const char* json_string);

// Helper functions
struct json_object* find_object_by_key(struct json_object* head, const char* key);
int count_objects(struct json_object* head);

// Object creation functions
struct json_object* create_string_object(const char* key, const char* value);
struct json_object* create_number_object(const char* key, double value);
struct json_object* create_boolean_object(const char* key, bool value);
struct json_object* create_null_object(const char* key);
struct json_object* create_empty_object(const char* key, bool is_array);
void append_object(struct json_object** head, struct json_object* new_obj);

// Recursive search functions - return arrays of matching values
char** get_string_values(struct json_root* root, const char* key, int* count);
double* get_number_values(struct json_root* root, const char* key, int* count);
bool* get_boolean_values(struct json_root* root, const char* key, int* count);
bool* get_null_values(struct json_root* root, const char* key, int* count);

// Serialization function - convert json_root back to JSON string
char* json_to_string(struct json_root* root);

#endif // JSON_H