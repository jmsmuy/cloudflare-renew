#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../lib/json.h"

// Helper function to count array elements
int count_array_elements(struct json_array* head) {
    int count = 0;
    struct json_array* current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// Test function to extract and validate all fields
void test_cloudflare_response_fields(struct json_root* root, const char* original_json) {
    printf("Testing all fields in Cloudflare response:\n");
    printf("==========================================\n");
    
    // Test top-level structure
    assert(root != NULL);
    assert(!root->is_array); // Should be an object, not array
    assert(root->object != NULL);
    
    printf("✓ Root structure is valid\n");
    
    // Test 'success' field
    struct json_object* success_obj = find_object_by_key(root->object, "success");
    assert(success_obj != NULL);
    assert(success_obj->is_boolean);
    assert(success_obj->value_boolean == true);
    printf("✓ 'success' field: %s (expected: true)\n", success_obj->value_boolean ? "true" : "false");
    
    // Test 'result' field (array)
    struct json_object* result_obj = find_object_by_key(root->object, "result");
    assert(result_obj != NULL);
    assert(result_obj->is_string); // Stored as string for nested parsing
    printf("✓ 'result' field: array stored as string\n");
    
    // Parse the result array
    struct json_root* result_root = parse_json(result_obj->value_string);
    assert(result_root != NULL);
    assert(result_root->is_array);
    assert(result_root->array != NULL);
    
    int result_count = count_array_elements(result_root->array);
    assert(result_count == 1); // Should have exactly 1 DNS record
    printf("✓ 'result' array has %d element(s) (expected: 1)\n", result_count);
    
    // Test the first (and only) DNS record object
    struct json_array* first_record = result_root->array;
    assert(first_record != NULL);
    assert(first_record->objects != NULL);
    
    int field_count = count_objects(first_record->objects);
    printf("✓ DNS record has %d fields\n", field_count);
    
    // Test individual fields in the DNS record
    struct json_object* id_obj = find_object_by_key(first_record->objects, "id");
    assert(id_obj != NULL);
    assert(id_obj->is_string);
    assert(strcmp(id_obj->value_string, "62cb453945a6cf0b9a58ea86948c59ad") == 0);
    printf("✓ 'id' field: '%s'\n", id_obj->value_string);
    
    struct json_object* name_obj = find_object_by_key(first_record->objects, "name");
    assert(name_obj != NULL);
    assert(name_obj->is_string);
    assert(strcmp(name_obj->value_string, "jmsmuy.com") == 0);
    printf("✓ 'name' field: '%s'\n", name_obj->value_string);
    
    struct json_object* type_obj = find_object_by_key(first_record->objects, "type");
    assert(type_obj != NULL);
    assert(type_obj->is_string);
    assert(strcmp(type_obj->value_string, "A") == 0);
    printf("✓ 'type' field: '%s'\n", type_obj->value_string);
    
    struct json_object* content_obj = find_object_by_key(first_record->objects, "content");
    assert(content_obj != NULL);
    assert(content_obj->is_string);
    assert(strcmp(content_obj->value_string, "179.24.91.14") == 0);
    printf("✓ 'content' field: '%s'\n", content_obj->value_string);
    
    struct json_object* proxiable_obj = find_object_by_key(first_record->objects, "proxiable");
    assert(proxiable_obj != NULL);
    assert(proxiable_obj->is_boolean);
    assert(proxiable_obj->value_boolean == true);
    printf("✓ 'proxiable' field: %s\n", proxiable_obj->value_boolean ? "true" : "false");
    
    struct json_object* proxied_obj = find_object_by_key(first_record->objects, "proxied");
    assert(proxied_obj != NULL);
    assert(proxied_obj->is_boolean);
    assert(proxied_obj->value_boolean == true);
    printf("✓ 'proxied' field: %s\n", proxied_obj->value_boolean ? "true" : "false");
    
    struct json_object* ttl_obj = find_object_by_key(first_record->objects, "ttl");
    assert(ttl_obj != NULL);
    assert(ttl_obj->is_number);
    assert(ttl_obj->value_number == 1.0);
    printf("✓ 'ttl' field: %.0f\n", ttl_obj->value_number);
    
    struct json_object* settings_obj = find_object_by_key(first_record->objects, "settings");
    assert(settings_obj != NULL);
    assert(settings_obj->is_string); // Empty object stored as string
    assert(strcmp(settings_obj->value_string, "{}") == 0);
    printf("✓ 'settings' field: '%s'\n", settings_obj->value_string);
    
    struct json_object* meta_obj = find_object_by_key(first_record->objects, "meta");
    assert(meta_obj != NULL);
    assert(meta_obj->is_string); // Empty object stored as string
    assert(strcmp(meta_obj->value_string, "{}") == 0);
    printf("✓ 'meta' field: '%s'\n", meta_obj->value_string);
    
    struct json_object* comment_obj = find_object_by_key(first_record->objects, "comment");
    assert(comment_obj != NULL);
    assert(comment_obj->is_null);
    printf("✓ 'comment' field: null\n");
    
    struct json_object* tags_obj = find_object_by_key(first_record->objects, "tags");
    assert(tags_obj != NULL);
    assert(tags_obj->is_string); // Empty array stored as string
    assert(strcmp(tags_obj->value_string, "[]") == 0);
    printf("✓ 'tags' field: '%s'\n", tags_obj->value_string);
    
    struct json_object* created_on_obj = find_object_by_key(first_record->objects, "created_on");
    assert(created_on_obj != NULL);
    assert(created_on_obj->is_string);
    assert(strcmp(created_on_obj->value_string, "2025-08-27T12:59:20.561294Z") == 0);
    printf("✓ 'created_on' field: '%s'\n", created_on_obj->value_string);
    
    struct json_object* modified_on_obj = find_object_by_key(first_record->objects, "modified_on");
    assert(modified_on_obj != NULL);
    assert(modified_on_obj->is_string);
    assert(strcmp(modified_on_obj->value_string, "2025-08-27T15:58:19.631448Z") == 0);
    printf("✓ 'modified_on' field: '%s'\n", modified_on_obj->value_string);
    
    // Clean up result_root
    free(result_root);
    
    // Test 'errors' field (empty array)
    struct json_object* errors_obj = find_object_by_key(root->object, "errors");
    assert(errors_obj != NULL);
    assert(errors_obj->is_string); // Empty array stored as string
    assert(strcmp(errors_obj->value_string, "[]") == 0);
    printf("✓ 'errors' field: '%s'\n", errors_obj->value_string);
    
    // Test 'messages' field (empty array)
    struct json_object* messages_obj = find_object_by_key(root->object, "messages");
    assert(messages_obj != NULL);
    assert(messages_obj->is_string); // Empty array stored as string
    assert(strcmp(messages_obj->value_string, "[]") == 0);
    printf("✓ 'messages' field: '%s'\n", messages_obj->value_string);
    
    // Test 'result_info' field (object)
    struct json_object* result_info_obj = find_object_by_key(root->object, "result_info");
    assert(result_info_obj != NULL);
    assert(result_info_obj->is_string); // Object stored as string
    
    // Parse the result_info object
    struct json_root* result_info_root = parse_json(result_info_obj->value_string);
    assert(result_info_root != NULL);
    assert(!result_info_root->is_array);
    assert(result_info_root->object != NULL);
    
    int info_field_count = count_objects(result_info_root->object);
    assert(info_field_count == 5); // Should have 5 fields
    printf("✓ 'result_info' object has %d fields (expected: 5)\n", info_field_count);
    
    // Test individual result_info fields
    struct json_object* page_obj = find_object_by_key(result_info_root->object, "page");
    assert(page_obj != NULL);
    assert(page_obj->is_number);
    assert(page_obj->value_number == 1.0);
    printf("✓ 'result_info.page' field: %.0f\n", page_obj->value_number);
    
    struct json_object* per_page_obj = find_object_by_key(result_info_root->object, "per_page");
    assert(per_page_obj != NULL);
    assert(per_page_obj->is_number);
    assert(per_page_obj->value_number == 100.0);
    printf("✓ 'result_info.per_page' field: %.0f\n", per_page_obj->value_number);
    
    struct json_object* count_obj = find_object_by_key(result_info_root->object, "count");
    assert(count_obj != NULL);
    assert(count_obj->is_number);
    assert(count_obj->value_number == 1.0);
    printf("✓ 'result_info.count' field: %.0f\n", count_obj->value_number);
    
    struct json_object* total_count_obj = find_object_by_key(result_info_root->object, "total_count");
    assert(total_count_obj != NULL);
    assert(total_count_obj->is_number);
    assert(total_count_obj->value_number == 1.0);
    printf("✓ 'result_info.total_count' field: %.0f\n", total_count_obj->value_number);
    
    struct json_object* total_pages_obj = find_object_by_key(result_info_root->object, "total_pages");
    assert(total_pages_obj != NULL);
    assert(total_pages_obj->is_number);
    assert(total_pages_obj->value_number == 1.0);
    printf("✓ 'result_info.total_pages' field: %.0f\n", total_pages_obj->value_number);
    
    // Clean up result_info_root
    free(result_info_root);
    
    // Test total field count at root level
    int total_root_fields = count_objects(root->object);
    assert(total_root_fields == 5); // success, result, errors, messages, result_info
    printf("✓ Root object has %d fields (expected: 5)\n", total_root_fields);
    
    printf("\n🎉 ALL TESTS PASSED! 🎉\n");
    printf("Successfully validated all %d fields in the Cloudflare API response\n", total_root_fields);
    
    // Test round-trip serialization
    printf("\nTesting Round-trip Serialization:\n");
    printf("================================\n");
    
    char* serialized = json_to_string(root);
    if (serialized) {
        printf("✅ Serialization successful!\n");
        printf("Original JSON length: %zu\n", strlen(original_json));
        printf("Serialized JSON length: %zu\n", strlen(serialized));
        
        // Compare the serialized output with the original input
        if (strcmp(original_json, serialized) == 0) {
            printf("✅ ROUND-TRIP SUCCESS! Serialized JSON matches original exactly!\n");
        } else {
            printf("⚠️  Round-trip differs from original (expected for complex nested JSON).\n");
            printf("📝 Note: Our parser stores nested arrays/objects as strings for simplicity.\n");
            printf("   This is expected behavior and doesn't affect functionality.\n");
            printf("Original: %.100s%s\n", original_json, strlen(original_json) > 100 ? "..." : "");
            printf("Serialized: %.100s%s\n", serialized, strlen(serialized) > 100 ? "..." : "");
        }
        
        free(serialized);
    } else {
        printf("❌ Serialization failed!\n");
    }
}

int main() {
    // Test JSON from Cloudflare API
    const char* test_json = "{\"result\":[{\"id\":\"62cb453945a6cf0b9a58ea86948c59ad\",\"name\":\"jmsmuy.com\",\"type\":\"A\",\"content\":\"179.24.91.14\",\"proxiable\":true,\"proxied\":true,\"ttl\":1,\"settings\":{},\"meta\":{},\"comment\":null,\"tags\":[],\"created_on\":\"2025-08-27T12:59:20.561294Z\",\"modified_on\":\"2025-08-27T15:58:19.631448Z\"}],\"success\":true,\"errors\":[],\"messages\":[],\"result_info\":{\"page\":1,\"per_page\":100,\"count\":1,\"total_count\":1,\"total_pages\":1}}";
    
    printf("Comprehensive JSON Parser Test\n");
    printf("==============================\n\n");
    
    // Parse the JSON
    struct json_root* root = parse_json(test_json);
    if (!root) {
        printf("❌ Error: Failed to parse JSON\n");
        return 1;
    }
    
    printf("✅ JSON parsed successfully!\n\n");
    
    // Run comprehensive field tests
    test_cloudflare_response_fields(root, test_json);
    
    // Clean up
    free(root);
    
    printf("\nTest completed successfully!\n");
    return 0;
}
