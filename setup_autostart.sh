#!/bin/bash

# ==============================================================
# G1_cloth_service Auto-Start Setup Script
# --------------------------------------------------------------
# This script sets up a systemd service to automatically start
# the cloth_service on system boot.
#
# Usage:
#   bash setup_autostart.sh
#
# After setup, manage the service with:
#   sudo systemctl status G1_cloth.service     # Check status
#   sudo journalctl -u G1_cloth.service -f     # View logs
#   sudo systemctl stop G1_cloth.service       # Stop service
#   sudo systemctl restart G1_cloth.service    # Restart service
#   sudo systemctl disable G1_cloth.service    # Disable auto-start on boot
# ==============================================================

set -e
echo "=== Setting up G1_cloth auto-start service ==="

SERVICE_NAME="G1_cloth.service"
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$PROJECT_DIR/bin"
ARCH="$(uname -m)"
SDK_LIB_DIR="$PROJECT_DIR/3rd/unitree_sdk2/thirdparty/lib/$ARCH"
echo "Binary directory detected: $BIN_DIR"
echo "SDK library directory: $SDK_LIB_DIR"

if [ ! -x "$BIN_DIR/cloth_service" ]; then
    echo "Error: $BIN_DIR/cloth_service not found. Please run 'bash build.sh' first."
    exit 1
fi

if [ ! -d "$SDK_LIB_DIR" ]; then
    echo "Error: SDK library directory $SDK_LIB_DIR does not exist."
    exit 1
fi

sudo tee $SERVICE_FILE > /dev/null << EOL
[Unit]
Description=G1 Cloth Service
After=multi-user.target

[Service]
Type=simple
User=unitree
Group=dialout
WorkingDirectory=$BIN_DIR
ExecStart=$BIN_DIR/cloth_service
Restart=always
RestartSec=5
Environment="LD_LIBRARY_PATH=$SDK_LIB_DIR"
StandardOutput=journal+console
StandardError=journal+console

[Install]
WantedBy=multi-user.target
EOL

echo "reloading systemd daemon..."
sudo systemctl daemon-reload

echo "enabling service to start on boot..."
sudo systemctl enable $SERVICE_NAME

echo "starting service now..."
sudo systemctl restart $SERVICE_NAME

echo "checking service status..."
sleep 2
sudo systemctl status $SERVICE_NAME --no-pager

echo ""
echo "=== G1_cloth auto-start service setup completed successfully! ==="
echo "You can manage the service using:"
echo "  sudo systemctl status $SERVICE_NAME      # Check status"
echo "  sudo journalctl -u $SERVICE_NAME -f      # View logs in real-time"
echo "  sudo systemctl restart $SERVICE_NAME     # Restart service"
echo "  sudo systemctl stop $SERVICE_NAME        # Stop service"
echo "  sudo systemctl disable $SERVICE_NAME     # Disable auto-start on boot"
echo "================================================================"
