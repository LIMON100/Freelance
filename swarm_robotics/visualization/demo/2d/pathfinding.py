import heapq
import math

class Node:
    def __init__(self, parent=None, position=None):
        self.parent = parent
        self.position = position
        self.g = 0
        self.h = 0
        self.f = 0

    def __eq__(self, other):
        return self.position == other.position

class PathFinder:
    def __init__(self, width, height, grid_size=20):
        self.width = width
        self.height = height
        self.grid_size = grid_size

    def get_grid_pos(self, pos):
        return (int(pos[0] // self.grid_size), int(pos[1] // self.grid_size))

    def get_world_pos(self, grid_pos):
        return (grid_pos[0] * self.grid_size + self.grid_size // 2, 
                grid_pos[1] * self.grid_size + self.grid_size // 2)

    def find_path(self, start_pos, end_pos, walls):
        start_node = Node(None, self.get_grid_pos(start_pos))
        end_node = Node(None, self.get_grid_pos(end_pos))

        open_list = []
        closed_list = set() # Use a set for fast lookup

        heapq.heappush(open_list, (0, id(start_node), start_node)) # Use id to break ties

        # Create obstacle map (simple grid check)
        # Optimization: Pre-calculating this map is faster, but checking on fly works for simple demo
        
        while len(open_list) > 0:
            current_node = heapq.heappop(open_list)[2]
            closed_list.add(current_node.position)

            # Found the goal
            if current_node == end_node:
                path = []
                current = current_node
                while current is not None:
                    path.append(self.get_world_pos(current.position))
                    current = current.parent
                return path[::-1] # Return reversed path

            # Generate children
            for new_position in [(0, -1), (0, 1), (-1, 0), (1, 0), (-1, -1), (-1, 1), (1, -1), (1, 1)]: # Adjacent squares
                node_position = (current_node.position[0] + new_position[0], current_node.position[1] + new_position[1])

                # Check within range
                if node_position[0] > (self.width // self.grid_size) - 1 or node_position[0] < 0 or node_position[1] > (self.height // self.grid_size) -1 or node_position[1] < 0:
                    continue

                # Check walkable (Collision with walls)
                # We check the world position of this grid cell against walls
                world_x, world_y = self.get_world_pos(node_position)
                collision = False
                robot_buffer = 25 # Treat robot as bigger than it is to avoid clipping corners
                
                # Check rect collision
                test_rect = (world_x - robot_buffer, world_y - robot_buffer, robot_buffer*2, robot_buffer*2)
                for wall in walls:
                    if wall.rect.colliderect(test_rect):
                        collision = True
                        break
                
                if collision:
                    continue

                if node_position in closed_list:
                    continue
                
                # Create new node
                new_node = Node(current_node, node_position)
                
                # Calculate costs
                new_node.g = current_node.g + 1
                # Heuristic (Euclidean distance)
                new_node.h = math.sqrt(((new_node.position[0] - end_node.position[0]) ** 2) + ((new_node.position[1] - end_node.position[1]) ** 2))
                new_node.f = new_node.g + new_node.h

                # Check if in open list with lower cost
                # (Skipping this strict check for speed in Python, usually fine for simple grids)
                
                heapq.heappush(open_list, (new_node.f, id(new_node), new_node))

        return None # No path found
