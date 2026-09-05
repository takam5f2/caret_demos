#!/bin/bash
set -e

# ========================================================================
# build_agnocast_demo.bash
#
# Builds caret_demos with agnocast integration enabled.
# After building, source setenv_caret_and_agnocast.bash to run the demo.
#
# Usage:
#   cd ~/caret_ws/caret_demos
#   bash caret_demos/scripts/build_agnocast_demo.bash
# ========================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CARET_DEMOS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source CARET environment (includes ROS 2 humble)
source /home/akm/ros2_caret_ws/setenv_caret.bash

# Source caret_demos workspace if exists
if [ -f "$CARET_DEMOS_DIR/install/setup.bash" ]; then
    source "$CARET_DEMOS_DIR/install/setup.bash"
fi

# Step: Build caret_demos
echo "=== Building caret_demos with agnocast ==="
cd "$CARET_DEMOS_DIR"
colcon build --symlink-install --cmake-args \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=Off \
    -DCMAKE_CXX_FLAGS="-w"
echo "=== caret_demos build complete ==="

echo "=== Done ==="
echo "To run the demo:"
echo "  cd $CARET_DEMOS_DIR"
echo "  source install/setup.bash"
echo "  ros2 launch caret_demos agnocast_demo.launch.py"
echo ""
echo "Or use the run script:"
echo "  bash $SCRIPT_DIR/run_agnocast_demo.bash"