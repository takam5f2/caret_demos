# CARET Demos

A collection of sample programs for testing trace and analysis with ROS 2 + CARET.

## Sample List

| Sample | Launch Command | Description |
|---------|-------------|------|
| end_to_end_sample | `ros2 launch caret_demos end_to_end_sample.launch.py` | Basic publish/subscribe chain |
| relay | `ros2 launch caret_demos end_to_end_sample_with_relay.launch.py` | Chain with a relay node |
| advanced_demo | `ros2 launch caret_demos advanced_demo.launch.py` | Multi-node demo |
| talker_listener | `ros2 launch caret_demos talker_listener.launch.py` | Simple 2-node communication |
| talker_listener_loaned_message | `ros2 launch caret_demos talker_listener_loaned_message.launch.py` | Communication using LoanedMessage |
| talker_listener_serialized_message | `ros2 launch caret_demos talker_listener_serialized_message.launch.py` | Communication using serialized messages |
| multi_talker_listener | `ros2 launch caret_demos multi_talker_listener.launch.py` | Multiple talker/listener |
| cyclic_pipeline_intra_process | `ros2 launch caret_demos cyclic_pipeline_intra_process.launch.py` | Cyclic pipeline with intra-process communication |
| agnocast_demo | `ros2 launch caret_demos agnocast_demo.launch.py` | Demo using agnocast (requires separate build) |

## Build

Please follow the instructions in caret_doc (See https://tier4.github.io/caret_doc/main/tutorials/recording/)


### Default build (without agnocast)

You can build “end_to_end_demo” and “advanced_demo.”

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws

git clone https://github.com/tier4/caret_demos.git src/caret_demos

source /opt/ros/jazzy/setup.bash

colcon build --symlink-install --packages-up-to caret_demos --cmake-args -DBUILD_TESTING=OFF
```

### Build with agnocast demo

```bash
colcon build --symlink-install --packages-up-to caret_demos --cmake-args -DBUILD_TESTING=OFF -DBUILD_AGNOCAST_DEMO=ON
```

## Trace Collection (with CARET)

### end_to_end_demo

**Terminal 1 (run demo):**
```bash
source ~/ros2_caret_ws/setenv_caret.bash
source ~/ros2_ws/install/local_setup.bash
ros2 launch caret_demos end_to_end_sample.launch.py
```

**Terminal 2 (trace recording):**
```bash
source /opt/ros/jazzy/setup.bash
source ~/ros2_caret_ws/install/local_setup.bash
ros2 caret record -v -s e2e-session
# Waits for "press enter to start..."
```

After Terminal 1 starts up, press Enter in Terminal 2 to begin tracing.
Stop with Ctrl+C. Trace data is saved to `~/.ros/tracing/e2e-session/` by default.

See `samples/end_to_end_sample/visualize_result.ipynb` for trace data analysis.

### advanced_demo

**Terminal 1 (run demo):**
```bash
source ~/ros2_caret_ws/setenv_caret.bash
source ~/ros2_ws/install/local_setup.bash
ros2 launch caret_demos advanced_demo.launch.py
```

**Terminal 2 (trace recording):**
```bash
source /opt/ros/jazzy/setup.bash
source ~/ros2_caret_ws/install/local_setup.bash
ros2 caret record -v -s advanced-session
```

### agnocast_demo

agnocast uses its own LD_PRELOAD (`libagnocast_heaphook.so`).
Both CARET's `libcaret.so` and agnocast's `libagnocast_heaphook.so` must be preloaded.

When sourcing agnocast's `local_setup.bash`, commands like `dirname` can break due to LD_PRELOAD.
To avoid this, `unset LD_PRELOAD` before sourcing, and set `LD_PRELOAD` at the end.

The “agnocast” package assumes that the source code has been installed in "~/agnocast" directory.
(See https://github.com/autowarefoundation/agnocast)

**Terminal 1 (run demo):**
```bash
unset LD_PRELOAD
source ~/ros2_caret_ws/setenv_caret.bash
source ~/agnocast/install/local_setup.bash
source ~/ros2_ws/install/local_setup.bash
export LD_PRELOAD=$HOME/agnocast/install/agnocastlib/lib/libagnocast_heaphook.so:$LD_PRELOAD
# Verify: both libagnocast_heaphook.so and libcaret.so should be present, in that order
echo $LD_PRELOAD
ros2 launch caret_demos agnocast_demo.launch.py
```

**Terminal 2 (trace recording):**
```bash
source /opt/ros/jazzy/setup.bash
source ~/ros2_caret_ws/install/local_setup.bash
ros2 caret record -v -s agnocast-session
```

## Analysis

#### For Analysis of end_to_end_demo
samples/jupyter/e2e_demo.ipynb

#### For Analysis of advanced_demo
samples/jupyter/advanced_demo.ipynb

#### For Analysis of agnocast_demo
samples/jupyter/agnocast_demo.ipynb

In the second cell of the notebook,
```
tracing_log_path = [
    ‘<path/to/caret_trace_data>’
],
```
so please set the full path to the collected trace files.

## Environment

- ROS 2 Humble / Jazzy
- CARET
- agnocast (optional)

## License

See [LICENSE](LICENSE).
