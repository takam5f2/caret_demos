import launch
from launch import LaunchDescription
from launch_ros.actions import Node
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_dir = get_package_share_directory('caret_demos')
    lib_dir = os.path.join(pkg_dir, 'lib', 'caret_demos')

    return LaunchDescription([
        # Pattern A nodes (rclcpp::Node + agnocast pub/sub) - 1 node per process
        Node(
            package='caret_demos',
            executable='agnocast_demo',
            arguments=['node1'],
            output='screen',
        ),
        Node(
            package='caret_demos',
            executable='agnocast_demo',
            arguments=['node2'],
            output='screen',
        ),
        Node(
            package='caret_demos',
            executable='agnocast_demo',
            arguments=['node3'],
            output='screen',
        ),
        Node(
            package='caret_demos',
            executable='agnocast_demo',
            arguments=['node6'],
            output='screen',
        ),
        Node(
            package='caret_demos',
            executable='agnocast_demo',
            arguments=['node7'],
            output='screen',
        ),

        # Pattern B node (agnocast::Node + agnocast pub/sub) - separate process
        Node(
            package='caret_demos',
            executable='agnocast_demo_pattern_b_relay',
            output='screen',
        ),

        # Pattern B + take node (agnocast::Node + PollingSubscriber) - separate process
        Node(
            package='caret_demos',
            executable='agnocast_demo_pattern_b_take',
            output='screen',
        ),
    ])
