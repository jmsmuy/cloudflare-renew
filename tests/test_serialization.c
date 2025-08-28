#include "../lib/json.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    // Test JSON from Cloudflare API
    const char *test_json =
        "{\"result\":[{\"id\":\"62cb453945a6cf0b9a58ea86948c59ad\",\"name\":\"jmsmuy.com\",\"type\":\"A\",\"content\":"
        "\"179.24.91.14\",\"proxiable\":true,\"proxied\":true,\"ttl\":1,\"settings\":{},\"meta\":{},\"comment\":null,"
        "\"tags\":[],\"created_on\":\"2025-08-27T12:59:20.561294Z\",\"modified_on\":\"2025-08-27T15:58:19.631448Z\"}],"
        "\"success\":true,\"errors\":[],\"messages\":[],\"result_info\":{\"page\":1,\"per_page\":100,\"count\":1,"
        "\"total_count\":1,\"total_pages\":1}}";

    printf("Testing JSON Serialization\n");
    printf("==========================\n\n");

    // Parse the JSON
    struct json_root *root = parse_json(test_json);
    if (!root) {
        printf("❌ Error: Failed to parse JSON\n");
        return 1;
    }

    printf("✅ JSON parsed successfully!\n\n");

    // Test serialization
    printf("Testing json_to_string():\n");
    printf("-------------------------\n");

    char *serialized = json_to_string(root);
    if (serialized) {
        printf("✅ Serialization successful!\n");
        printf("Serialized JSON: %s\n", serialized);
        free(serialized);
    } else {
        printf("❌ Serialization failed!\n");
    }

    // Clean up
    free(root);

    printf("\nTest completed!\n");
    return 0;
}
