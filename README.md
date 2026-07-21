# ROS2 Action Examples

This repository demonstrates how to implement ROS 2 Actions in both Python and C++.

## Packages

### action_py
Python implementation of:
- Action Server
- Action Client

### actions_cpp
C++ implementation of:
- Action Server
- Action Client

## Requirements

- Ubuntu 24.04
- ROS 2 Jazzy

## Build

```bash
cd ~/ros2_ws
colcon build
source install/setup.bash
```

## Run Python Server

```bash
ros2 run action_py move_robot_server
```

## Run Python Client

```bash
ros2 run action_py move_robot_client
```

## Run C++ Server

```bash
ros2 run actions_cpp move_robot_server
```

## Run C++ Client

```bash
ros2 run actions_cpp move_robot_client
```
