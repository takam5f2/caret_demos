#!/bin/bash
set -e

# ========================================================================
# run_agnocast_demo.bash
#
# Launches the agnocast demo with CARET tracing enabled.
# Usage:
#   ./run_agnocast_demo.bash [agnocast_workspace_path]
# Example:
#   ./run_agnocast_demo.bash ~/agnocast
# ========================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CARET_DEMOS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source caret_demos workspace
if [ -f "$CARET_DEMOS_DIR/install/setup.bash" ]; then
    source "$CARET_DEMOS_DIR/install/setup.bash"
fi

# ========================================================================
# libagnocast_heaphook.so のパス解決
# ========================================================================
AGNOCAST_HEAPHOOK_SO=""

if [ -n "$1" ]; then
    # 1. 引数が指定された場合: 指定されたパスをベースに探索
    #    (~ などのチルダを展開するため eval を使用)
    CUSTOM_DIR=$(eval echo "$1")
    CANDIDATE="$CUSTOM_DIR/install/agnocastlib/lib/libagnocast_heaphook.so"

    if [ -f "$CANDIDATE" ]; then
        AGNOCAST_HEAPHOOK_SO="$CANDIDATE"
    else
        echo "[ERROR] 指定されたパスに libagnocast_heaphook.so が見つかりません:" >&2
        echo "  -> $CANDIDATE" >&2
        exit 1
    fi
else
    # 2. 引数未指定の場合: ROS_DISTRO に従ったシステム標準パスを使用
    if [ -z "$ROS_DISTRO" ]; then
        echo "[ERROR] ROS_DISTRO が設定されていません。ROS 2 環境が読み込まれているか確認してください。" >&2
        exit 1
    fi

    CANDIDATE="/opt/ros/${ROS_DISTRO}/lib/libagnocast_heaphook.so"

    if [ -f "$CANDIDATE" ]; then
        AGNOCAST_HEAPHOOK_SO="$CANDIDATE"
    else
        echo "[ERROR] システム標準のパスに libagnocast_heaphook.so が見つかりません:" >&2
        echo "  -> $CANDIDATE" >&2
        echo "ヒント: カスタムビルド版を使用する場合は、引数にパスを指定してください (例: $0 ~/agnocast)" >&2
        exit 1
    fi
fi

# LD_PRELOAD の設定
export LD_PRELOAD="$AGNOCAST_HEAPHOOK_SO"

# Source CARET environment (sets LD_PRELOAD with libcaret.so)
source /home/akm/ros2_caret_ws/setenv_caret.bash

echo "=== LD_PRELOAD ==="
echo "$LD_PRELOAD"
echo "=================="

# Run the demo
ros2 launch caret_demos agnocast_demo.launch.py

