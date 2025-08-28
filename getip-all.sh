#!/bin/bash

# Script to get IP for all configured domains
# Usage: ./getip-all.sh

CONFIG_FILE="cloudflare.conf"
TOKEN_FILE="cloudflare.token"

echo "🌐 Getting IP addresses for all configured domains..."
echo "=================================================="

# Extract all domain names from the config file
domains=$(grep "DOMAIN_NAME\[" "$CONFIG_FILE" | grep -v "^#" | sed 's/.*=\(.*\)/\1/')

if [ -z "$domains" ]; then
    echo "❌ No domains found in configuration file"
    exit 1
fi

# Loop through each domain and get its IP
while IFS= read -r domain; do
    if [ -n "$domain" ]; then
        echo "📡 $domain:"
        ip=$(./tools/getip "$CONFIG_FILE" "$TOKEN_FILE" "$domain" 2>/dev/null)
        if [ $? -eq 0 ] && [ -n "$ip" ]; then
            echo "   ✅ $ip"
        else
            echo "   ❌ Failed to get IP"
        fi
        echo
    fi
done <<< "$domains"

echo "🎯 Summary - Current public IP: $(./tools/publicip)"
