import heapq
import math
import pygame

class Node:
    def __init__(self, parent=None, position=None):
        self.parent = parent
        self.position = position
        self.g = 0
        self.h = 0
        self.f = 0

    def __eq__(self, other):
        return self.position == other.position
        
    def __lt__(self, other):
        return self.f < other.f
        
    def __hash__(self):
        return hash(self.position)

class PathFinder:
    def __init__(self, width, height, grid_size=10): # CHANGED TO 10 (Higher Resolution)
        self.width = width
        self.height = height
        self.grid_size = grid_size
        print(f"[PathFinder] Initialized with Grid Size: {grid_size}")

    def get_grid_pos(self, pos):
        return (int(pos[0] // self.grid_size), int(pos[1] // self.grid_size))

    def get_world_pos(self, grid_pos):
        return (grid_pos[0] * self.grid_size + self.grid_size // 2, 
                grid_pos[1] * self.grid_size + self.grid_size // 2)

    def is_collision(self, grid_pos, walls):
        world_x, world_y = self.get_world_pos(grid_pos)
        
        # Robot Radius is 15. Let's use 16 as buffer.
        buffer = 16 
        
        test_rect = pygame.Rect(world_x - buffer, world_y - buffer, buffer*2, buffer*2)
        
        for wall in walls:
            if wall.rect.colliderect(test_rect):
                return True
        return False

    def get_valid_node(self, grid_pos, walls):
        if not self.is_collision(grid_pos, walls):
            return grid_pos
        
        # Increased Search Radius to 15 to escape thicker walls
        x, y = grid_pos
        for r in range(1, 15): 
            for i in range(-r, r + 1):
                for j in range(-r, r + 1):
                    new_pos = (x + i, y + j)
                    if 0 <= new_pos[0] < (self.width // self.grid_size) and \
                       0 <= new_pos[1] < (self.height // self.grid_size):
                        if not self.is_collision(new_pos, walls):
                            return new_pos
        return None 

    def find_path(self, start_pos, end_pos, walls):
        start_grid = self.get_grid_pos(start_pos)
        end_grid = self.get_grid_pos(end_pos)

        start_grid = self.get_valid_node(start_grid, walls)
        end_grid = self.get_valid_node(end_grid, walls)

        if start_grid is None or end_grid is None:
            return []

        start_node = Node(None, start_grid)
        end_node = Node(None, end_grid)

        open_list = []
        closed_set = set()
        open_dict = {} 

        heapq.heappush(open_list, (0, id(start_node), start_node))
        open_dict[start_node.position] = start_node
        
        best_node = start_node
        min_dist_to_goal = float('inf')

        max_iterations = 10000 # Increased for finer grid
        iter_count = 0

        while len(open_list) > 0:
            iter_count += 1
            if iter_count > max_iterations:
                return self.reconstruct_path(best_node)

            current_node = heapq.heappop(open_list)[2]
            
            if current_node.position in closed_set:
                continue
            
            if current_node.position in open_dict:
                del open_dict[current_node.position]
            closed_set.add(current_node.position)

            if current_node == end_node:
                return self.reconstruct_path(current_node)
            
            dist = math.sqrt((current_node.position[0] - end_node.position[0])**2 + 
                             (current_node.position[1] - end_node.position[1])**2)
            if dist < min_dist_to_goal:
                min_dist_to_goal = dist
                best_node = current_node

            for new_position in [(0, -1), (0, 1), (-1, 0), (1, 0), (-1, -1), (-1, 1), (1, -1), (1, 1)]:
                node_position = (current_node.position[0] + new_position[0], current_node.position[1] + new_position[1])

                if not (0 <= node_position[0] < (self.width // self.grid_size) and 0 <= node_position[1] < (self.height // self.grid_size)):
                    continue
                if self.is_collision(node_position, walls):
                    continue
                if node_position in closed_set:
                    continue

                new_node = Node(current_node, node_position)
                
                # Diagonal Cost = 1.4, Straight = 1.0
                move_cost = 1.414 if new_position[0] != 0 and new_position[1] != 0 else 1.0
                new_node.g = current_node.g + move_cost
                
                new_node.h = math.sqrt(((new_node.position[0] - end_node.position[0]) ** 2) + 
                                     ((new_node.position[1] - end_node.position[1]) ** 2))
                new_node.f = new_node.g + new_node.h

                if node_position in open_dict and open_dict[node_position].g <= new_node.g:
                    continue
                
                heapq.heappush(open_list, (new_node.f, id(new_node), new_node))
                open_dict[node_position] = new_node

        return self.reconstruct_path(best_node)

    def reconstruct_path(self, current_node):
        path = []
        while current_node is not None:
            path.append(self.get_world_pos(current_node.position))
            current_node = current_node.parent
        return path[::-1]
    

    # Add this method to the PathFinder class
    def generate_lawnmower_path(self, rect, walls):
        """Generates a zigzag/lawnmower path within a given rectangle."""
        path = []
        if not rect:
            return path

        x, y, w, h = rect.x, rect.y, rect.width, rect.height
        step = self.grid_size * 2  # How far apart are the rows
        
        # Start at top-left of the rectangle
        start_node = self.get_valid_node(self.get_grid_pos((x, y)), walls)
        if not start_node: return []
        
        current_y = self.get_world_pos(start_node)[1]
        
        direction = 1  # 1 for right, -1 for left
        
        while current_y < y + h:
            # Determine start and end x for this row
            start_x = x if direction == 1 else x + w
            end_x = x + w if direction == 1 else x
            
            # Find path to the start of the row
            start_of_row_path = self.find_path((path[-1][0], path[-1][1]) if path else self.get_world_pos(start_node), (start_x, current_y), walls)
            if start_of_row_path:
                path.extend(start_of_row_path)
            
            # Find path along the row
            end_of_row_path = self.find_path((start_x, current_y), (end_x, current_y), walls)
            if end_of_row_path:
                path.extend(end_of_row_path)

            # Move to next row and reverse direction
            current_y += step
            direction *= -1
                
        print(f"[PathFinder] Generated Lawnmower Path with {len(path)} waypoints.")
        return path