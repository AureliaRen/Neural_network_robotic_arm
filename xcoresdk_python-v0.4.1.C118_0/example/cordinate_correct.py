import numpy as np
from scipy.spatial.transform import Rotation as R

def world_to_robot_coordinates_with_euler(world_coords, world_euler):
    """
    Convert world coordinates and Euler angles to robot coordinates and Euler angles
    accounting for a rotation around the z-axis by arctan(1/3).

    Args:
        world_coords (tuple or list): Position in the world frame (x, y, z).
        world_euler (tuple or list): Euler angles in the world frame (roll, pitch, yaw) in radians.

    Returns:
        tuple: Robot frame position (x, y, z) and Euler angles (roll, pitch, yaw) in radians.
    """
    # Rotation angle (in radians)
    theta = np.arctan(1 / 3)

    # Rotation matrix around z-axis
    rotation_matrix = np.array([
        [np.cos(theta), -np.sin(theta), 0],
        [np.sin(theta),  np.cos(theta), 0],
        [0,              0,             1]
    ])

    # Convert input position to numpy array
    world_coords = np.array(world_coords)

    # Validate position input dimension
    if world_coords.shape != (3,):
        raise ValueError("Position input must be a 3D coordinate (x, y, z).")

    # Apply rotation to position
    robot_coords = rotation_matrix @ world_coords

    # Convert Euler angles to rotation matrix
    world_rotation = R.from_euler('xyz', world_euler)

    # Combine world rotation with z-axis rotation
    z_axis_rotation = R.from_matrix(rotation_matrix)
    robot_rotation = z_axis_rotation * world_rotation

    # Convert robot rotation back to Euler angles
    robot_euler = robot_rotation.as_euler('xyz')

    return robot_coords, robot_euler

# Example usage
if __name__ == "__main__":
    # Example world coordinates and Euler angles
    world_coords = (0.445, 0.187, 0.434)
    world_euler = (np.pi/2, 0, np.pi/2)  # Roll, pitch, yaw in radians

    # Convert to robot frame
    robot_coords, robot_euler = world_to_robot_coordinates_with_euler(world_coords, world_euler)

    print("World Coordinates:", world_coords)
    print("World Euler Angles (rad):", world_euler)
    print("Robot Coordinates:", robot_coords)
    print("Robot Euler Angles (rad):", robot_euler)
