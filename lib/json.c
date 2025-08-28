#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Helper function to skip whitespace
static char *skip_whitespace(char *str)
{
    while (*str && isspace(*str)) {
        str++;
    }
    return str;
}

// Helper function to parse a JSON string value
static char *parse_string(char **json_ptr)
{
    char *start = *json_ptr;
    if (*start != '"') {
        return NULL;
    }
    start++; // Skip opening quote

    char *end = start;
    while (*end && *end != '"') {
        if (*end == '\\' && *(end + 1)) {
            end += 2; // Skip escaped character
        } else {
            end++;
        }
    }

    if (*end != '"') {
        return NULL;
    }

    size_t len = end - start;
    char *result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    strncpy(result, start, len);
    result[len] = '\0';

    *json_ptr = end + 1; // Move pointer past closing quote
    return result;
}

// Helper function to parse a JSON number
static double parse_number(char **json_ptr)
{
    char *start = *json_ptr;
    char *end = start;

    // Handle negative numbers
    if (*end == '-') {
        end++;
    }

    // Parse digits
    while (*end && isdigit(*end)) {
        end++;
    }

    // Handle decimal point
    if (*end == '.') {
        end++;
        while (*end && isdigit(*end))
            end++;
    }

    // Handle scientific notation
    if (*end == 'e' || *end == 'E') {
        end++;
        if (*end == '+' || *end == '-')
            end++;
        while (*end && isdigit(*end))
            end++;
    }

    char *num_str = malloc(end - start + 1);
    if (!num_str)
        return 0.0;

    strncpy(num_str, start, end - start);
    num_str[end - start] = '\0';

    char *endptr;
    double result = strtod(num_str, &endptr);
    free(num_str);

    *json_ptr = end;
    return result;
}

// Helper function to parse a JSON boolean or null
static bool parse_boolean_or_null(char **json_ptr, bool *is_null, bool *value)
{
    char *start = *json_ptr;

    if (strncmp(start, "true", 4) == 0) {
        *is_null = false;
        *value = true;
        *json_ptr = start + 4;
        return true;
    } else if (strncmp(start, "false", 5) == 0) {
        *is_null = false;
        *value = false;
        *json_ptr = start + 5;
        return true;
    } else if (strncmp(start, "null", 4) == 0) {
        *is_null = true;
        *value = false;
        *json_ptr = start + 4;
        return true;
    }

    return false;
}

// Parse a JSON object
static struct json_object *parse_object(char **json_ptr)
{
    char *start = *json_ptr;
    if (*start != '{')
        return NULL;
    start++; // Skip opening brace
    start = skip_whitespace(start);

    struct json_object *head = NULL;
    struct json_object *current = NULL;

    while (*start && *start != '}') {
        start = skip_whitespace(start);

        // Parse key
        char *key = parse_string(&start);
        if (!key)
            break;

        start = skip_whitespace(start);
        if (*start != ':') {
            free(key);
            break;
        }
        start++; // Skip colon
        start = skip_whitespace(start);

        // Create new object
        struct json_object *obj = malloc(sizeof(struct json_object));
        if (!obj) {
            free(key);
            break;
        }

        obj->key = key;
        obj->next = NULL;
        obj->is_string = false;
        obj->is_number = false;
        obj->is_boolean = false;
        obj->is_null = false;

        // Parse value based on type
        if (*start == '"') {
            obj->value_string = parse_string(&start);
            obj->is_string = true;
        } else if (*start == '[') {
            // Handle arrays - for now, we'll store them as strings
            // In a more complete implementation, we'd store a pointer to the array
            const char *array_start = start;
            int bracket_count = 0;
            while (*start) {
                if (*start == '[')
                    bracket_count++;
                else if (*start == ']')
                    bracket_count--;
                start++;
                if (bracket_count == 0)
                    break;
            }
            size_t array_len = start - array_start;
            obj->value_string = malloc(array_len + 1);
            if (obj->value_string) {
                strncpy(obj->value_string, array_start, array_len);
                obj->value_string[array_len] = '\0';
                obj->is_string = true;
            }
        } else if (*start == '{') {
            // Handle nested objects - for now, we'll store them as strings
            const char *obj_start = start;
            int brace_count = 0;
            while (*start) {
                if (*start == '{')
                    brace_count++;
                else if (*start == '}')
                    brace_count--;
                start++;
                if (brace_count == 0)
                    break;
            }
            size_t obj_len = start - obj_start;
            obj->value_string = malloc(obj_len + 1);
            if (obj->value_string) {
                strncpy(obj->value_string, obj_start, obj_len);
                obj->value_string[obj_len] = '\0';
                obj->is_string = true;
            }
        } else if (isdigit(*start) || *start == '-') {
            obj->value_number = parse_number(&start);
            obj->is_number = true;
        } else if (*start == 't' || *start == 'f' || *start == 'n') {
            bool is_null, bool_value;
            if (parse_boolean_or_null(&start, &is_null, &bool_value)) {
                if (is_null) {
                    obj->is_null = true;
                } else {
                    obj->value_boolean = bool_value;
                    obj->is_boolean = true;
                }
            }
        }

        // Add to linked list
        if (!head) {
            head = obj;
            current = obj;
        } else {
            current->next = obj;
            current = obj;
        }

        start = skip_whitespace(start);
        if (*start == ',') {
            start++; // Skip comma
        }
    }

    if (*start == '}') {
        start++; // Skip closing brace
    }

    *json_ptr = start;
    return head;
}

// Parse a JSON array
static struct json_array *parse_array(char **json_ptr)
{
    char *start = *json_ptr;
    if (*start != '[')
        return NULL;
    start++; // Skip opening bracket
    start = skip_whitespace(start);

    struct json_array *head = NULL;
    struct json_array *current = NULL;

    while (*start && *start != ']') {
        start = skip_whitespace(start);

        // Create new array element
        struct json_array *arr_elem = malloc(sizeof(struct json_array));
        if (!arr_elem)
            break;

        arr_elem->objects = NULL;
        arr_elem->next = NULL;

        // Parse the element (could be object, string, number, etc.)
        if (*start == '{') {
            arr_elem->objects = parse_object(&start);
        } else if (*start == '"') {
            // For simple values in arrays, we'll store them as objects with empty key
            struct json_object *obj = malloc(sizeof(struct json_object));
            if (obj) {
                obj->key = strdup("");
                obj->value_string = parse_string(&start);
                obj->is_string = true;
                obj->is_number = false;
                obj->is_boolean = false;
                obj->is_null = false;
                obj->next = NULL;
                arr_elem->objects = obj;
            }
        }

        // Add to linked list
        if (!head) {
            head = arr_elem;
            current = arr_elem;
        } else {
            current->next = arr_elem;
            current = arr_elem;
        }

        start = skip_whitespace(start);
        if (*start == ',') {
            start++; // Skip comma
        }
    }

    if (*start == ']') {
        start++; // Skip closing bracket
    }

    *json_ptr = start;
    return head;
}

// Main parsing function
struct json_root *parse_json(const char *json_string)
{
    if (!json_string)
        return NULL;

    char *json_ptr = (char *) json_string;
    json_ptr = skip_whitespace(json_ptr);

    struct json_root *root = malloc(sizeof(struct json_root));
    if (!root)
        return NULL;

    root->object = NULL;
    root->array = NULL;
    root->is_array = false;

    if (*json_ptr == '{') {
        root->object = parse_object(&json_ptr);
        root->is_array = false;
    } else if (*json_ptr == '[') {
        root->array = parse_array(&json_ptr);
        root->is_array = true;
    }

    return root;
}

// Helper function to find a json_object by key
struct json_object *find_object_by_key(struct json_object *head, const char *key)
{
    struct json_object *current = head;
    while (current) {
        if (current->key && strcmp(current->key, key) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Helper function to count objects in a linked list
int count_objects(struct json_object *head)
{
    int count = 0;
    struct json_object *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// Recursive function to search for values in objects and arrays
static void
search_string_values_recursive(struct json_object *obj, const char *key, char ***results, int *count, int *capacity)
{
    if (!obj)
        return;

    // Check if this object matches the key
    if (obj->key && strcmp(obj->key, key) == 0 && obj->is_string) {
        // Resize array if needed
        if (*count >= *capacity) {
            *capacity = (*capacity == 0) ? 4 : *capacity * 2;
            *results = realloc(*results, *capacity * sizeof(char *));
        }
        (*results)[*count] = strdup(obj->value_string);
        (*count)++;
    }

    // Recursively search in nested objects and arrays
    if (obj->is_string) {
        // Try to parse as JSON and search recursively
        struct json_root *nested = parse_json(obj->value_string);
        if (nested) {
            if (nested->is_array) {
                struct json_array *arr = nested->array;
                while (arr) {
                    if (arr->objects) {
                        search_string_values_recursive(arr->objects, key, results, count, capacity);
                    }
                    arr = arr->next;
                }
            } else if (nested->object) {
                search_string_values_recursive(nested->object, key, results, count, capacity);
            }
            free(nested);
        }
    }

    // Continue to next object in the list
    search_string_values_recursive(obj->next, key, results, count, capacity);
}

static void
search_number_values_recursive(struct json_object *obj, const char *key, double **results, int *count, int *capacity)
{
    if (!obj)
        return;

    // Check if this object matches the key
    if (obj->key && strcmp(obj->key, key) == 0 && obj->is_number) {
        // Resize array if needed
        if (*count >= *capacity) {
            *capacity = (*capacity == 0) ? 4 : *capacity * 2;
            *results = realloc(*results, *capacity * sizeof(double));
        }
        (*results)[*count] = obj->value_number;
        (*count)++;
    }

    // Recursively search in nested objects and arrays
    if (obj->is_string) {
        // Try to parse as JSON and search recursively
        struct json_root *nested = parse_json(obj->value_string);
        if (nested) {
            if (nested->is_array) {
                struct json_array *arr = nested->array;
                while (arr) {
                    if (arr->objects) {
                        search_number_values_recursive(arr->objects, key, results, count, capacity);
                    }
                    arr = arr->next;
                }
            } else if (nested->object) {
                search_number_values_recursive(nested->object, key, results, count, capacity);
            }
            free(nested);
        }
    }

    // Continue to next object in the list
    search_number_values_recursive(obj->next, key, results, count, capacity);
}

static void
search_boolean_values_recursive(struct json_object *obj, const char *key, bool **results, int *count, int *capacity)
{
    if (!obj)
        return;

    // Check if this object matches the key
    if (obj->key && strcmp(obj->key, key) == 0 && obj->is_boolean) {
        // Resize array if needed
        if (*count >= *capacity) {
            *capacity = (*capacity == 0) ? 4 : *capacity * 2;
            *results = realloc(*results, *capacity * sizeof(bool));
        }
        (*results)[*count] = obj->value_boolean;
        (*count)++;
    }

    // Recursively search in nested objects and arrays
    if (obj->is_string) {
        // Try to parse as JSON and search recursively
        struct json_root *nested = parse_json(obj->value_string);
        if (nested) {
            if (nested->is_array) {
                struct json_array *arr = nested->array;
                while (arr) {
                    if (arr->objects) {
                        search_boolean_values_recursive(arr->objects, key, results, count, capacity);
                    }
                    arr = arr->next;
                }
            } else if (nested->object) {
                search_boolean_values_recursive(nested->object, key, results, count, capacity);
            }
            free(nested);
        }
    }

    // Continue to next object in the list
    search_boolean_values_recursive(obj->next, key, results, count, capacity);
}

static void
search_null_values_recursive(struct json_object *obj, const char *key, bool **results, int *count, int *capacity)
{
    if (!obj)
        return;

    // Check if this object matches the key
    if (obj->key && strcmp(obj->key, key) == 0 && obj->is_null) {
        // Resize array if needed
        if (*count >= *capacity) {
            *capacity = (*capacity == 0) ? 4 : *capacity * 2;
            *results = realloc(*results, *capacity * sizeof(bool));
        }
        (*results)[*count] = true; // All null values are represented as true
        (*count)++;
    }

    // Recursively search in nested objects and arrays
    if (obj->is_string) {
        // Try to parse as JSON and search recursively
        struct json_root *nested = parse_json(obj->value_string);
        if (nested) {
            if (nested->is_array) {
                struct json_array *arr = nested->array;
                while (arr) {
                    if (arr->objects) {
                        search_null_values_recursive(arr->objects, key, results, count, capacity);
                    }
                    arr = arr->next;
                }
            } else if (nested->object) {
                search_null_values_recursive(nested->object, key, results, count, capacity);
            }
            free(nested);
        }
    }

    // Continue to next object in the list
    search_null_values_recursive(obj->next, key, results, count, capacity);
}

// Public API functions
char **get_string_values(struct json_root *root, const char *key, int *count)
{
    if (!root || !key || !count)
        return NULL;

    *count = 0;
    char **results = NULL;
    int capacity = 0;

    if (root->is_array) {
        // Search in array elements
        struct json_array *arr = root->array;
        while (arr) {
            if (arr->objects) {
                search_string_values_recursive(arr->objects, key, &results, count, &capacity);
            }
            arr = arr->next;
        }
    } else if (root->object) {
        // Search in object
        search_string_values_recursive(root->object, key, &results, count, &capacity);
    }

    return results;
}

double *get_number_values(struct json_root *root, const char *key, int *count)
{
    if (!root || !key || !count)
        return NULL;

    *count = 0;
    double *results = NULL;
    int capacity = 0;

    if (root->is_array) {
        // Search in array elements
        struct json_array *arr = root->array;
        while (arr) {
            if (arr->objects) {
                search_number_values_recursive(arr->objects, key, &results, count, &capacity);
            }
            arr = arr->next;
        }
    } else if (root->object) {
        // Search in object
        search_number_values_recursive(root->object, key, &results, count, &capacity);
    }

    return results;
}

bool *get_boolean_values(struct json_root *root, const char *key, int *count)
{
    if (!root || !key || !count)
        return NULL;

    *count = 0;
    bool *results = NULL;
    int capacity = 0;

    if (root->is_array) {
        // Search in array elements
        struct json_array *arr = root->array;
        while (arr) {
            if (arr->objects) {
                search_boolean_values_recursive(arr->objects, key, &results, count, &capacity);
            }
            arr = arr->next;
        }
    } else if (root->object) {
        // Search in object
        search_boolean_values_recursive(root->object, key, &results, count, &capacity);
    }

    return results;
}

bool *get_null_values(struct json_root *root, const char *key, int *count)
{
    if (!root || !key || !count)
        return NULL;

    *count = 0;
    bool *results = NULL;
    int capacity = 0;

    if (root->is_array) {
        // Search in array elements
        struct json_array *arr = root->array;
        while (arr) {
            if (arr->objects) {
                search_null_values_recursive(arr->objects, key, &results, count, &capacity);
            }
            arr = arr->next;
        }
    } else if (root->object) {
        // Search in object
        search_null_values_recursive(root->object, key, &results, count, &capacity);
    }

    return results;
}

// Object creation functions
struct json_object *create_string_object(const char *key, const char *value)
{
    struct json_object *obj = malloc(sizeof(struct json_object));
    if (!obj)
        return NULL;

    obj->key = strdup(key);
    obj->value_string = strdup(value);
    obj->value_number = 0.0;
    obj->value_boolean = false;
    obj->value_null = false;
    obj->is_string = true;
    obj->is_number = false;
    obj->is_boolean = false;
    obj->is_null = false;
    obj->next = NULL;

    return obj;
}

struct json_object *create_number_object(const char *key, double value)
{
    struct json_object *obj = malloc(sizeof(struct json_object));
    if (!obj)
        return NULL;

    obj->key = strdup(key);
    obj->value_string = NULL;
    obj->value_number = value;
    obj->value_boolean = false;
    obj->value_null = false;
    obj->is_string = false;
    obj->is_number = true;
    obj->is_boolean = false;
    obj->is_null = false;
    obj->next = NULL;

    return obj;
}

struct json_object *create_boolean_object(const char *key, bool value)
{
    struct json_object *obj = malloc(sizeof(struct json_object));
    if (!obj)
        return NULL;

    obj->key = strdup(key);
    obj->value_string = NULL;
    obj->value_number = 0.0;
    obj->value_boolean = value;
    obj->value_null = false;
    obj->is_string = false;
    obj->is_number = false;
    obj->is_boolean = true;
    obj->is_null = false;
    obj->next = NULL;

    return obj;
}

// cppcheck-suppress unusedFunction
struct json_object *create_null_object(const char *key)
{
    struct json_object *obj = malloc(sizeof(struct json_object));
    if (!obj)
        return NULL;

    obj->key = strdup(key);
    obj->value_string = NULL;
    obj->value_number = 0.0;
    obj->value_boolean = false;
    obj->value_null = true;
    obj->is_string = false;
    obj->is_number = false;
    obj->is_boolean = false;
    obj->is_null = true;
    obj->next = NULL;

    return obj;
}

// cppcheck-suppress unusedFunction
struct json_object *create_empty_object(const char *key, bool is_array)
{
    struct json_object *obj = malloc(sizeof(struct json_object));
    if (!obj)
        return NULL;

    obj->key = strdup(key);
    obj->value_string = strdup(is_array ? "[]" : "{}");
    obj->value_number = 0.0;
    obj->value_boolean = false;
    obj->value_null = false;
    obj->is_string = true;
    obj->is_number = false;
    obj->is_boolean = false;
    obj->is_null = false;
    obj->next = NULL;

    return obj;
}

void append_object(struct json_object **head, struct json_object *new_obj)
{
    if (!*head) {
        *head = new_obj;
        return;
    }

    struct json_object *current = *head;
    while (current->next) {
        current = current->next;
    }
    current->next = new_obj;
}

// Helper function to serialize a json_object to string
static char *object_to_string(struct json_object *obj)
{
    if (!obj)
        return strdup("{}");

    // Calculate total length needed
    size_t total_len = 2; // for { and }
    struct json_object *current = obj;
    int field_count = 0;

    while (current) {
        if (current->key) {
            total_len += strlen(current->key) + 4; // "key":
            if (current->is_string && current->value_string) {
                total_len += strlen(current->value_string) + 2; // "value"
            } else if (current->is_number) {
                total_len += 20; // enough for most numbers
            } else if (current->is_boolean) {
                total_len += current->value_boolean ? 4 : 5; // true or false
            } else if (current->is_null) {
                total_len += 4; // null
            }
        }
        current = current->next;
        field_count++;
    }

    if (field_count > 1) {
        total_len += field_count - 1; // commas between fields
    }

    char *result = malloc(total_len + 1);
    if (!result)
        return NULL;

    strcpy(result, "{");
    current = obj;
    bool first = true;

    while (current) {
        if (current->key) {
            if (!first) {
                strcat(result, ",");
            }
            first = false;

            strcat(result, "\"");
            strcat(result, current->key);
            strcat(result, "\":");

            if (current->is_string && current->value_string) {
                strcat(result, "\"");
                strcat(result, current->value_string);
                strcat(result, "\"");
            } else if (current->is_number) {
                char num_str[32];
                snprintf(num_str, sizeof(num_str), "%.0f", current->value_number);
                strcat(result, num_str);
            } else if (current->is_boolean) {
                strcat(result, current->value_boolean ? "true" : "false");
            } else if (current->is_null) {
                strcat(result, "null");
            }
        }
        current = current->next;
    }

    strcat(result, "}");
    return result;
}

// Helper function to serialize a json_array to string
static char *array_to_string(struct json_array *arr)
{
    if (!arr)
        return strdup("[]");

    // Calculate total length needed
    size_t total_len = 2; // for [ and ]
    struct json_array *current = arr;
    int element_count = 0;

    while (current) {
        if (current->objects) {
            char *obj_str = object_to_string(current->objects);
            if (obj_str) {
                total_len += strlen(obj_str);
                free(obj_str);
            }
        }
        current = current->next;
        element_count++;
    }

    if (element_count > 1) {
        total_len += element_count - 1; // commas between elements
    }

    char *result = malloc(total_len + 1);
    if (!result)
        return NULL;

    strcpy(result, "[");
    current = arr;
    bool first = true;

    while (current) {
        if (!first) {
            strcat(result, ",");
        }
        first = false;

        if (current->objects) {
            char *obj_str = object_to_string(current->objects);
            if (obj_str) {
                strcat(result, obj_str);
                free(obj_str);
            } else {
                strcat(result, "{}");
            }
        } else {
            strcat(result, "{}");
        }

        current = current->next;
    }

    strcat(result, "]");
    return result;
}

// Main serialization function
char *json_to_string(struct json_root *root)
{
    if (!root)
        return NULL;

    if (root->is_array) {
        return array_to_string(root->array);
    } else {
        return object_to_string(root->object);
    }
}
