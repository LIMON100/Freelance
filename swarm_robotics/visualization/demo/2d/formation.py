import math
import pygame
from pathfinding import PathFinder

class FormationManager:
    def __init__(self, leader, followers):
        self.leader = leader
        self.followers = followers
        self.all_robots = [leader] + followers
        
        # Initialize PathFinder (Map size 1280x720)
        self.pathfinder = PathFinder(1280, 720)
        self.current_path = [] # Store the list of points the leader follows
        self.path_timer = 0    # Don't recalculate every single frame (too slow)

        # Formation Offsets
        self.offsets = [
            (-60, -50), (-60, 50), (-120, -100), (-120, 100)
        ]

    def update(self, target_obj, walls):
        # --- 1. Leader Pathfinding ---
        
        # Recalculate path every 30 frames (0.5 seconds) OR if we have no path
        self.path_timer += 1
        if self.path_timer > 30 or not self.current_path:
            self.path_timer = 0
            # Calculate path from Leader to Target
            # We assume walls don't move instantly.
            # NOTE: We pass the 'walls' object list to the pathfinder
            path = self.pathfinder.find_path((self.leader.x, self.leader.y), target_obj.rect.center, walls)
            if path:
                self.current_path = path

        # --- 2. Move Leader along Path ---
        if self.current_path:
            # The next immediate target is the first point in the path list
            next_point = self.current_path[0]
            
            # Distance to next waypoint
            dist = math.sqrt((next_point[0]-self.leader.x)**2 + (next_point[1]-self.leader.y)**2)
            
            # If close to waypoint, remove it and go to next
            if dist < 20: 
                self.current_path.pop(0)
            
            # Move towards the waypoint
            if self.current_path: # Check again in case we popped the last one
                self.move_robot_simple(self.leader, self.current_path[0])
            else:
                # Arrived at final target
                self.leader.vx, self.leader.vy = 0, 0

        # --- 3. Leader Heading ---
        if self.leader.vx != 0 or self.leader.vy != 0:
            heading = math.atan2(self.leader.vy, self.leader.vx)
        else:
            dx = target_obj.rect.center[0] - self.leader.x
            dy = target_obj.rect.center[1] - self.leader.y
            heading = math.atan2(dy, dx)

        # --- 4. Update Followers (Standard Logic) ---
        for i, follower in enumerate(self.followers):
            off_x, off_y = self.offsets[i]
            rot_x = off_x * math.cos(heading) - off_y * math.sin(heading)
            rot_y = off_x * math.sin(heading) + off_y * math.cos(heading)
            
            slot_x = self.leader.x + rot_x
            slot_y = self.leader.y + rot_y
            
            # Followers can use simple movement because they just follow the leader
            # (In a real swarm, they would also use A*, but for this demo, direct follow is usually ok)
            # However, to prevent them clipping walls, we can use the simple repulsion logic here
            self.move_robot_with_avoidance(follower, (slot_x, slot_y), walls)

    def move_robot_simple(self, robot, goal):
        """Pure movement to point (for Leader following A* nodes)"""
        dx = goal[0] - robot.x
        dy = goal[1] - robot.y
        dist = math.sqrt(dx**2 + dy**2)
        if dist > 0:
            speed = 3.0
            robot.vx = (dx/dist) * speed
            robot.vy = (dy/dist) * speed
            robot.x += robot.vx
            robot.y += robot.vy

    def move_robot_with_avoidance(self, robot, goal, walls):
        """Followers use this: Try to go to slot, but push away from walls"""
        dx = goal[0] - robot.x
        dy = goal[1] - robot.y
        dist = math.sqrt(dx**2 + dy**2)
        
        force_x, force_y = 0, 0
        
        if dist > 5:
            force_x += dx/dist
            force_y += dy/dist
            
        # Wall Repulsion
        for wall in walls:
            closest_x = max(wall.rect.left, min(robot.x, wall.rect.right))
            closest_y = max(wall.rect.top, min(robot.y, wall.rect.bottom))
            w_dx = robot.x - closest_x
            w_dy = robot.y - closest_y
            w_dist = math.sqrt(w_dx**2 + w_dy**2)
            
            if w_dist < 40:
                 if w_dist == 0: w_dist = 0.1
                 force_x += (w_dx/w_dist) * 5.0
                 force_y += (w_dy/w_dist) * 5.0
                 
        # Apply
        mag = math.sqrt(force_x**2 + force_y**2)
        speed = 3.0
        if mag > 0:
            robot.vx = (force_x/mag) * speed
            robot.vy = (force_y/mag) * speed
            robot.x += robot.vx
            robot.y += robot.vy
