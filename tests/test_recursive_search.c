#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../lib/json.h"

int main() {
    // Test JSON from Cloudflare API
    const char* test_json = "{\"result\":[{\"id\":\"62cb453945a6cf0b9a58ea86948c59ad\",\"name\":\"jmsmuy.com\",\"type\":\"A\",\"content\":\"179.24.91.14\",\"proxiable\":true,\"proxied\":true,\"ttl\":1,\"settings\":{},\"meta\":{},\"comment\":null,\"tags\":[],\"created_on\":\"2025-08-27T12:59:20.561294Z\",\"modified_on\":\"2025-08-27T15:58:19.631448Z\"}],\"success\":true,\"errors\":[],\"messages\":[],\"result_info\":{\"page\":1,\"per_page\":100,\"count\":1,\"total_count\":1,\"total_pages\":1}}";
    
    printf("Testing Recursive Search Functions\n");
    printf("==================================\n\n");
    
    // Parse the JSON
    struct json_root* root = parse_json(test_json);
    if (!root) {
        printf("❌ Error: Failed to parse JSON\n");
        return 1;
    }
    
    printf("✅ JSON parsed successfully!\n\n");
    
    // Test string values search
    printf("Testing get_string_values():\n");
    printf("----------------------------\n");
    
    int count = 0;
    char** string_values = get_string_values(root, "content", &count);
    assert(string_values != NULL);
    assert(count == 1);
    assert(strcmp(string_values[0], "179.24.91.14") == 0);
    printf("✓ Found %d 'content' values: '%s'\n", count, string_values[0]);
    
    // Test name field
    char** name_values = get_string_values(root, "name", &count);
    assert(name_values != NULL);
    assert(count == 1);
    assert(strcmp(name_values[0], "jmsmuy.com") == 0);
    printf("✓ Found %d 'name' values: '%s'\n", count, name_values[0]);
    
    // Test type field
    char** type_values = get_string_values(root, "type", &count);
    assert(type_values != NULL);
    assert(count == 1);
    assert(strcmp(type_values[0], "A") == 0);
    printf("✓ Found %d 'type' values: '%s'\n", count, type_values[0]);
    
    // Test boolean values search
    printf("\nTesting get_boolean_values():\n");
    printf("-----------------------------\n");
    
    bool* bool_values = get_boolean_values(root, "success", &count);
    assert(bool_values != NULL);
    assert(count == 1);
    assert(bool_values[0] == true);
    printf("✓ Found %d 'success' values: %s\n", count, bool_values[0] ? "true" : "false");
    
    bool* proxiable_values = get_boolean_values(root, "proxiable", &count);
    assert(proxiable_values != NULL);
    assert(count == 1);
    assert(proxiable_values[0] == true);
    printf("✓ Found %d 'proxiable' values: %s\n", count, proxiable_values[0] ? "true" : "false");
    
    bool* proxied_values = get_boolean_values(root, "proxied", &count);
    assert(proxied_values != NULL);
    assert(count == 1);
    assert(proxied_values[0] == true);
    printf("✓ Found %d 'proxied' values: %s\n", count, proxied_values[0] ? "true" : "false");
    
    // Test number values search
    printf("\nTesting get_number_values():\n");
    printf("----------------------------\n");
    
    double* number_values = get_number_values(root, "ttl", &count);
    assert(number_values != NULL);
    assert(count == 1);
    assert(number_values[0] == 1.0);
    printf("✓ Found %d 'ttl' values: %.0f\n", count, number_values[0]);
    
    double* page_values = get_number_values(root, "page", &count);
    assert(page_values != NULL);
    assert(count == 1);
    assert(page_values[0] == 1.0);
    printf("✓ Found %d 'page' values: %.0f\n", count, page_values[0]);
    
    double* per_page_values = get_number_values(root, "per_page", &count);
    assert(per_page_values != NULL);
    assert(count == 1);
    assert(per_page_values[0] == 100.0);
    printf("✓ Found %d 'per_page' values: %.0f\n", count, per_page_values[0]);
    
    // Test null values search
    printf("\nTesting get_null_values():\n");
    printf("--------------------------\n");
    
    bool* null_values = get_null_values(root, "comment", &count);
    assert(null_values != NULL);
    assert(count == 1);
    assert(null_values[0] == true);
    printf("✓ Found %d 'comment' null values\n", count);
    
    // Test multiple values (should find multiple "count" fields)
    printf("\nTesting multiple values:\n");
    printf("------------------------\n");
    
    double* count_values = get_number_values(root, "count", &count);
    assert(count_values != NULL);
    printf("Found %d 'count' values:\n", count);
    for (int i = 0; i < count; i++) {
        printf("  [%d]: %.0f\n", i, count_values[i]);
    }
    // Note: The JSON only has one "count" field in result_info, not two
    assert(count == 1);
    assert(count_values[0] == 1.0);
    
    // Clean up
    free(string_values[0]);
    free(string_values);
    free(name_values[0]);
    free(name_values);
    free(type_values[0]);
    free(type_values);
    free(bool_values);
    free(proxiable_values);
    free(proxied_values);
    free(number_values);
    free(page_values);
    free(per_page_values);
    free(null_values);
    free(count_values);
    free(root);
    
    printf("\n🎉 ALL RECURSIVE SEARCH TESTS PASSED! 🎉\n");
    printf("The new API successfully finds values at any depth in the JSON structure!\n");
    
    return 0;
}
