import math
import pygame

class FormationManager:
    """
    Handles Swarm Logic with improved Collision Avoidance.
    """
    def __init__(self, leader, followers):
        self.leader = leader
        self.followers = followers
        # Combine all robots for collision checks
        self.all_robots = [leader] + followers
        
        # Formation Settings (Distance from Leader)
        # Format: (Distance Back, Distance Right)
        self.offsets = [
            (-60, -50), # Follower 1: Left Rear
            (-60, 50),  # Follower 2: Right Rear
            (-120, -100), # Follower 3: Far Left Rear
            (-120, 100)   # Follower 4: Far Right Rear
        ]

    def update(self, target_obj, walls):
        """
        Main update loop for the swarm.
        """
        # --- 1. Update Leader ---
        self.move_robot(self.leader, target_obj.rect.center, walls)

        # --- 2. Calculate Leader's Heading ---
        if self.leader.vx != 0 or self.leader.vy != 0:
            heading = math.atan2(self.leader.vy, self.leader.vx)
        else:
            # If stopped, face the target
            dx = target_obj.rect.center[0] - self.leader.x
            dy = target_obj.rect.center[1] - self.leader.y
            heading = math.atan2(dy, dx)

        # --- 3. Update Followers ---
        for i, follower in enumerate(self.followers):
            # Calculate Virtual Slot
            off_x, off_y = self.offsets[i]
            
            # Rotate offset based on leader's heading
            rotated_x = off_x * math.cos(heading) - off_y * math.sin(heading)
            rotated_y = off_x * math.sin(heading) + off_y * math.cos(heading)
            
            slot_x = self.leader.x + rotated_x
            slot_y = self.leader.y + rotated_y
            
            self.move_robot(follower, (slot_x, slot_y), walls)


    def move_robot(self, robot, goal_pos, walls):
        """
        Enhanced Navigation Logic.
        1. Attractive Force (Goal)
        2. Repulsive Force (Walls)
        3. Repulsive Force (Other Robots) - NEW
        4. Hard Collision Check (Stop at Wall) - NEW
        """
        
        # --- A. Forces Calculation ---
        
        # 1. Goal Attraction
        dx = goal_pos[0] - robot.x
        dy = goal_pos[1] - robot.y
        dist = math.sqrt(dx**2 + dy**2)
        
        if dist < 5:
            robot.vx, robot.vy = 0, 0
            return

        force_x = dx / dist
        force_y = dy / dist
        
        # 2. Wall Repulsion (Soft Push)
        wall_repulse_x, wall_repulse_y = 0, 0
        wall_safety_radius = 60
        
        for wall in walls:
            # Get closest point on wall rectangle to robot center
            closest_x = max(wall.rect.left, min(robot.x, wall.rect.right))
            closest_y = max(wall.rect.top, min(robot.y, wall.rect.bottom))
            
            w_dx = robot.x - closest_x
            w_dy = robot.y - closest_y
            w_dist = math.sqrt(w_dx**2 + w_dy**2)
            
            if w_dist < wall_safety_radius:
                if w_dist == 0: w_dist = 0.1 # Prevent div by zero
                # Stronger repulsion as we get closer
                strength = (wall_safety_radius - w_dist) / wall_safety_radius
                wall_repulse_x += (w_dx / w_dist) * strength * 8.0 
                wall_repulse_y += (w_dy / w_dist) * strength * 8.0

        # 3. Robot Repulsion (Don't hit friends!) - NEW
        robot_repulse_x, robot_repulse_y = 0, 0
        robot_separation_radius = 40 # Minimum comfortable distance
        
        for other in self.all_robots:
            if other is robot: continue # Don't repel self
            
            r_dx = robot.x - other.x
            r_dy = robot.y - other.y
            r_dist = math.sqrt(r_dx**2 + r_dy**2)
            
            if r_dist < robot_separation_radius:
                if r_dist == 0: r_dist = 0.1
                strength = (robot_separation_radius - r_dist) / robot_separation_radius
                # Very strong repulsion to prevent overlapping
                robot_repulse_x += (r_dx / r_dist) * strength * 5.0
                robot_repulse_y += (r_dy / r_dist) * strength * 5.0

        # --- B. Combine & Move ---
        
        # Combine all forces
        total_fx = force_x + wall_repulse_x + robot_repulse_x
        total_fy = force_y + wall_repulse_y + robot_repulse_y
        
        # Apply Speed
        speed = 2.0
        # Normalize result vector to keep constant speed (unless stopped)
        total_mag = math.sqrt(total_fx**2 + total_fy**2)
        if total_mag > 0:
            move_x = (total_fx / total_mag) * speed
            move_y = (total_fy / total_mag) * speed
        else:
            move_x, move_y = 0, 0

        # --- C. Hard Collision Check (The Wall Stop) ---
        
        # Proposed new position
        next_x = robot.x + move_x
        next_y = robot.y + move_y
        
        # Create a temporary rectangle for the robot at new pos
        robot_rect = pygame.Rect(next_x - 15, next_y - 15, 30, 30) # Assuming radius 15
        
        can_move_x = True
        can_move_y = True
        
        for wall in walls:
            if wall.rect.colliderect(robot_rect):
                # We hit a wall!
                # Simple logic: Stop moving.
                # Advanced logic (sliding) is harder, but stopping prevents "Running over".
                
                # Check X collision separately
                rect_x = pygame.Rect(next_x - 15, robot.y - 15, 30, 30)
                if wall.rect.colliderect(rect_x):
                    can_move_x = False
                
                # Check Y collision separately
                rect_y = pygame.Rect(robot.x - 15, next_y - 15, 30, 30)
                if wall.rect.colliderect(rect_y):
                    can_move_y = False

        # Apply allowed movement
        if can_move_x:
            robot.x += move_x
            robot.vx = move_x
        else:
            robot.vx = 0 # Hit wall in X
            
        if can_move_y:
            robot.y += move_y
            robot.vy = move_y
        else:
            robot.vy = 0 # Hit wall in Y