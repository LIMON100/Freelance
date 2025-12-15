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

    # FIX 1: Add a hash function so the Node object can be a key in a dictionary/set
    def __hash__(self):
        return hash(self.position)

class PathFinder:
    def __init__(self, width, height, grid_size=25):
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
        buffer = 20
        test_rect = pygame.Rect(world_x - buffer, world_y - buffer, buffer*2, buffer*2)
        for wall in walls:
            if wall.rect.colliderect(test_rect):
                return True
        return False

    def get_valid_node(self, grid_pos, walls):
        if not self.is_collision(grid_pos, walls):
            return grid_pos
        print(f"[PathFinder] Point {grid_pos} is blocked. Searching...")
        for r in range(1, 15):
            for i in range(-r, r + 1):
                for j in range(-r, r + 1):
                    new_pos = (grid_pos[0] + i, grid_pos[1] + j)
                    if 0 <= new_pos[0] < (self.width // self.grid_size) and \
                       0 <= new_pos[1] < (self.height // self.grid_size):
                        if not self.is_collision(new_pos, walls):
                            print(f"[PathFinder] Found valid neighbor at {new_pos}")
                            return new_pos
        print("[PathFinder] CRITICAL: No free spot nearby!")
        return None 

    def find_path(self, start_pos, end_pos, walls):
        start_grid = self.get_grid_pos(start_pos)
        end_grid = self.get_grid_pos(end_pos)

        start_grid = self.get_valid_node(start_grid, walls)
        end_grid = self.get_valid_node(end_grid, walls)

        if start_grid is None or end_grid is None:
            print("[PathFinder] Failed: Invalid start or end.")
            return []

        start_node = Node(None, start_grid)
        end_node = Node(None, end_grid)

        open_list = []
        closed_set = set()
        open_dict = {} # Tracks nodes currently in the priority queue

        heapq.heappush(open_list, (start_node.f, id(start_node), start_node)) # Use id() to make items unique for heapq
        open_dict[start_node.position] = start_node

        best_node = start_node
        min_dist_to_goal = float('inf')

        max_iterations = 5000
        iter_count = 0

        while len(open_list) > 0:
            iter_count += 1
            if iter_count > max_iterations:
                print("[PathFinder] Max Iterations. Returning partial path.")
                return self.reconstruct_path(best_node)

            current_node = heapq.heappop(open_list)[2] # Get the Node object
            
            # FIX 2: If we already pulled this position from the open list, skip it.
            # This handles duplicates that can occur in a simple heapq implementation.
            if current_node.position in closed_set:
                continue
            
            # Move from open to closed
            if current_node.position in open_dict:
                del open_dict[current_node.position]
            closed_set.add(current_node.position)

            if current_node == end_node:
                print(f"[PathFinder] Path Found! Length: {current_node.g}")
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
                move_cost = math.sqrt((new_node.position[0] - current_node.position[0])**2 + (new_node.position[1] - current_node.position[1])**2)
                new_node.g = current_node.g + move_cost
                new_node.h = math.sqrt(((new_node.position[0] - end_node.position[0]) ** 2) + 
                                     ((new_node.position[1] - end_node.position[1]) ** 2))
                new_node.f = new_node.g + new_node.h

                # FIX 3: Check if the node is already in open_list and if the new path is better.
                if node_position in open_dict and open_dict[node_position].g <= new_node.g:
                    continue
                
                heapq.heappush(open_list, (new_node.f, id(new_node), new_node))
                open_dict[node_position] = new_node

        print("[PathFinder] No complete path found. Returning partial path.")
        return self.reconstruct_path(best_node)

    def reconstruct_path(self, current_node):
        path = []
        while current_node is not None:
            path.append(self.get_world_pos(current_node.position))
            current_node = current_node.parent
        return path[::-1] if path else []