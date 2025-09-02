#!/bin/sh
echo "🚀 Cloudflare DNS Renew OpenWrt Setup"
echo "====================================="

# Make binaries executable
chmod +x cloudflare_renew getip setip publicip getip-all.sh setip-all.sh

# Check if config exists
if [ ! -f cloudflare.conf ]; then
  echo "📋 Creating cloudflare.conf from template..."
  cp cloudflare.conf.sample cloudflare.conf
  echo "⚠️  Please edit cloudflare.conf with your Zone ID, DNS Record ID, and Domain Name"
fi

# Check if token exists
if [ ! -f cloudflare.token ]; then
  echo "🔑 Please create cloudflare.token with your Cloudflare API token"
  echo "   Get your token from: https://dash.cloudflare.com/profile/api-tokens"
fi

echo ""
echo "✅ Setup complete! Available commands:"
echo "   ./cloudflare_renew          - Automatic DNS management"
echo "   ./getip <config> <token>    - Get current DNS IP"
echo "   ./setip <config> <token> <ip> - Set DNS IP"
echo "   ./publicip                  - Get public IP"
echo ""
echo "📖 See README.md for detailed documentation"
