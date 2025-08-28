#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../lib/json.h"

int main() {
    // Test with a simpler JSON that doesn't have nested structures stored as strings
    const char* test_json = "{\"name\":\"test\",\"value\":42,\"active\":true,\"data\":null}";
    
    printf("Testing Simple Round-trip Serialization\n");
    printf("=======================================\n\n");
    
    // Parse the JSON
    struct json_root* root = parse_json(test_json);
    if (!root) {
        printf("❌ Error: Failed to parse JSON\n");
        return 1;
    }
    
    printf("✅ JSON parsed successfully!\n\n");
    
    // Test serialization
    printf("Testing json_to_string():\n");
    printf("-------------------------\n");
    
    char* serialized = json_to_string(root);
    if (serialized) {
        printf("✅ Serialization successful!\n");
        printf("Original JSON: %s\n", test_json);
        printf("Serialized JSON: %s\n", serialized);
        printf("Original length: %zu\n", strlen(test_json));
        printf("Serialized length: %zu\n", strlen(serialized));
        
        // Compare the serialized output with the original input
        if (strcmp(test_json, serialized) == 0) {
            printf("✅ ROUND-TRIP SUCCESS! Serialized JSON matches original exactly!\n");
        } else {
            printf("❌ Round-trip failed! Serialized JSON differs from original.\n");
        }
        
        free(serialized);
    } else {
        printf("❌ Serialization failed!\n");
    }
    
    // Clean up
    free(root);
    
    printf("\nTest completed!\n");
    return 0;
}
