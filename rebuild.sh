#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "=== Building dynamic-cursors with click animation ==="
make all

echo "=== Installing plugin to hyprpm cache ==="
sudo install -m 755 out/dynamic-cursors.so /var/cache/hyprpm/sensei/dynamic-cursors/dynamic-cursors.so
mkdir -p ~/.local/share/hyprpm-custom-plugins
install -m 755 out/dynamic-cursors.so ~/.local/share/hyprpm-custom-plugins/dynamic-cursors.so

echo "=== Hot-reloading in Hyprland ==="
hyprpm reload
hyprctl reload

echo "=== Done! Custom dynamic-cursors reloaded and active. ==="
