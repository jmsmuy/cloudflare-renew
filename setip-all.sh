#!/bin/bash

# Script to set IP for all configured domains
# Usage: ./setip-all.sh <ip_address>

if [ $# -ne 1 ]; then
    echo "Usage: $0 <ip_address>"
    echo "Example: $0 192.168.1.100"
    echo "         $0 \$(./tools/publicip)  # Use current public IP"
    exit 1
fi

IP_ADDRESS="$1"
CONFIG_FILE="cloudflare.conf"
TOKEN_FILE="cloudflare.token"

echo "🌐 Setting IP address '$IP_ADDRESS' for all configured domains..."
echo "================================================================"

# Validate IP address format (basic validation)
if ! echo "$IP_ADDRESS" | grep -E '^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$' >/dev/null; then
    echo "❌ Invalid IP address format: $IP_ADDRESS"
    exit 1
fi

# Extract all domain names from the config file
domains=$(grep "DOMAIN_NAME\[" "$CONFIG_FILE" | grep -v "^#" | sed 's/.*=\(.*\)/\1/')

if [ -z "$domains" ]; then
    echo "❌ No domains found in configuration file"
    exit 1
fi

success_count=0
total_count=0

# Loop through each domain and set its IP
while IFS= read -r domain; do
    if [ -n "$domain" ]; then
        echo "🔄 Updating $domain..."
        total_count=$((total_count + 1))
        
        if ./tools/setip "$CONFIG_FILE" "$TOKEN_FILE" "$IP_ADDRESS" "$domain" >/dev/null 2>&1; then
            echo "   ✅ Successfully updated to $IP_ADDRESS"
            success_count=$((success_count + 1))
        else
            echo "   ❌ Failed to update"
            # Show the actual error for debugging
            echo "   Error details:"
            ./tools/setip "$CONFIG_FILE" "$TOKEN_FILE" "$IP_ADDRESS" "$domain" 2>&1 | sed 's/^/      /'
        fi
        echo
    fi
done <<< "$domains"

echo "📊 Summary:"
echo "   ✅ Successfully updated: $success_count/$total_count domains"
echo "   🎯 Target IP: $IP_ADDRESS"
echo "   🌍 Current public IP: $(./tools/publicip)"

if [ $success_count -eq $total_count ]; then
    echo "   🎉 All domains updated successfully!"
    exit 0
else
    echo "   ⚠️  Some domains failed to update"
    exit 1
fi
