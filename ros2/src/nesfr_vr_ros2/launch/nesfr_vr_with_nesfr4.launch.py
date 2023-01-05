from launch import LaunchDescription
import launch_ros.actions


def generate_launch_description():
    return LaunchDescription([
        launch_ros.actions.Node(
            package='nesfr_vr_ros2', executable='nesfr_vr_ros2', output='screen'),
    ])
