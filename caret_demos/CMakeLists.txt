cmake_minimum_required(VERSION 3.8)
project(caret_demos)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(end_to_end_sample
  src/end_to_end_sample.cpp)
ament_target_dependencies(end_to_end_sample rclcpp sensor_msgs)

add_executable(relay
  src/relay.cpp)
ament_target_dependencies(relay rclcpp sensor_msgs)

add_executable(advanced_demo src/advanced_demo.cpp)
ament_target_dependencies(advanced_demo rclcpp sensor_msgs)

install(TARGETS
  end_to_end_sample
  relay
  advanced_demo
  DESTINATION lib/${PROJECT_NAME})

# install launch files
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
  )

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
