cmake_minimum_required(VERSION 3.8)
project(caret_demos)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

include_directories(SYSTEM 
  ${tracetools_INCLUDE_DIRS}
  ${rclcpp_INCLUDE_DIRS}
)

# Build option for agnocast demo (default OFF)
option(BUILD_AGNOCAST_DEMO "Build agnocast demo targets" OFF)

if(BUILD_AGNOCAST_DEMO)
  find_package(agnocastlib REQUIRED)
endif()

add_executable(end_to_end_sample src/end_to_end_sample.cpp)
ament_target_dependencies(end_to_end_sample rclcpp sensor_msgs)

add_executable(relay src/relay.cpp)
ament_target_dependencies(relay rclcpp sensor_msgs)

add_executable(advanced_demo src/advanced_demo.cpp)
ament_target_dependencies(advanced_demo
    rclcpp
    sensor_msgs
)

if(BUILD_AGNOCAST_DEMO)
  add_executable(agnocast_demo src/agnocast_demo.cpp)
  ament_target_dependencies(agnocast_demo
      rclcpp
      sensor_msgs
      agnocastlib
  )
  target_include_directories(agnocast_demo PRIVATE
      ${agnocastlib_INCLUDE_DIRS}
  )

  add_executable(agnocast_demo_pattern_b_relay src/agnocast_demo_pattern_b_relay.cpp)
  ament_target_dependencies(agnocast_demo_pattern_b_relay
      rclcpp
      sensor_msgs
      agnocastlib
  )
  target_include_directories(agnocast_demo_pattern_b_relay PRIVATE
      ${agnocastlib_INCLUDE_DIRS}
  )

  add_executable(agnocast_demo_pattern_b_take src/agnocast_demo_pattern_b_take.cpp)
  ament_target_dependencies(agnocast_demo_pattern_b_take
      rclcpp
      sensor_msgs
      agnocastlib
  )
  target_include_directories(agnocast_demo_pattern_b_take PRIVATE
      ${agnocastlib_INCLUDE_DIRS}
  )

  # --- agnocast: one node per process ---
  add_executable(agnocast_demo_node1_sensor src/agnocast_demo_node1_sensor.cpp)
  ament_target_dependencies(agnocast_demo_node1_sensor rclcpp sensor_msgs)

  add_executable(agnocast_demo_node2_bridge_to src/agnocast_demo_node2_bridge_to.cpp)
  ament_target_dependencies(agnocast_demo_node2_bridge_to rclcpp sensor_msgs agnocastlib)
  target_include_directories(agnocast_demo_node2_bridge_to PRIVATE ${agnocastlib_INCLUDE_DIRS})

  add_executable(agnocast_demo_node3_relay_a src/agnocast_demo_node3_relay_a.cpp)
  ament_target_dependencies(agnocast_demo_node3_relay_a rclcpp sensor_msgs agnocastlib)
  target_include_directories(agnocast_demo_node3_relay_a PRIVATE ${agnocastlib_INCLUDE_DIRS})

  add_executable(agnocast_demo_node6_bridge_from src/agnocast_demo_node6_bridge_from.cpp)
  ament_target_dependencies(agnocast_demo_node6_bridge_from rclcpp sensor_msgs agnocastlib)
  target_include_directories(agnocast_demo_node6_bridge_from PRIVATE ${agnocastlib_INCLUDE_DIRS})

  add_executable(agnocast_demo_node7_actuator src/agnocast_demo_node7_actuator.cpp)
  ament_target_dependencies(agnocast_demo_node7_actuator rclcpp sensor_msgs)
endif()

install(TARGETS
  end_to_end_sample
  relay
  advanced_demo
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_AGNOCAST_DEMO)
  install(TARGETS
    agnocast_demo
    agnocast_demo_pattern_b_relay
    agnocast_demo_pattern_b_take
    agnocast_demo_node1_sensor
    agnocast_demo_node2_bridge_to
    agnocast_demo_node3_relay_a
    agnocast_demo_node6_bridge_from
    agnocast_demo_node7_actuator
    DESTINATION lib/${PROJECT_NAME})
endif()

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

