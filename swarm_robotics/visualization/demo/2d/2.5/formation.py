import math
import pygame
import random
from collections import deque
from pathfinding import PathFinder

class FollowerState:
    FORMATION = 1
    REGROUP = 2

class FormationManager:
    def __init__(self, leader, followers, map_width, map_height):
        self.leader = leader
        self.followers = followers
        self.all_robots = [leader] + followers
        
        self.pathfinder = PathFinder(map_width, map_height)
        
        # --- STANDARDIZED NAMING ---
        self.leader_path = [] # This is the A* path for the LEADER
        self.path_timer = 0
        # ---------------------------

        self.leader_history = deque(maxlen=300) 
        
        self.follower_states = {f: FollowerState.FORMATION for f in followers}
        self.follower_paths = {f: [] for f in followers}
        
        self.stuck_counters = {r: 0 for r in self.all_robots}

    def switch_leader(self, new_leader_index):
        """
        Swaps the current leader with a new robot from the swarm.
        """
        # 1. Reset current leader to follower
        self.leader.color = (50, 100, 255) # Blue
        self.leader.is_leader = False
        
        # 2. Assign new leader
        self.leader = self.all_robots[new_leader_index]
        self.leader.color = (255, 50, 50) # Red
        self.leader.is_leader = True
        
        # 3. Rebuild Followers List (Everyone except the new leader)
        self.followers = [r for r in self.all_robots if r != self.leader]
        
        # 4. Reset Pathing & History for new leader
        self.current_path = []
        self.leader_history.clear()
        
        # 5. Reset Follower States (So they don't get confused)
        self.follower_states = {f: 1 for f in self.followers} # Reset to FORMATION state
        self.follower_paths = {f: [] for f in self.followers}
        
        print(f"[Formation] Leader Switched to Robot ID: {self.leader.id}")

    def update(self, target_obj, walls, user_waypoints):
        # 1. LEADER LOGIC
        leader_goal = None
        
        if user_waypoints:
            next_wp = user_waypoints[0]
            dist_to_wp = math.sqrt((self.leader.x - next_wp[0])**2 + (self.leader.y - next_wp[1])**2)
            if dist_to_wp < 30:
                user_waypoints.pop(0)
            leader_goal = next_wp
            self.leader_path = [] # Manual override, clear A*
        else:
            final_destination = target_obj.rect.center
            self.path_timer += 1
            if self.path_timer > 30 or not self.leader_path: # USE leader_path
                self.path_timer = 0
                raw_path = self.pathfinder.find_path((self.leader.x, self.leader.y), final_destination, walls)
                if raw_path:
                    self.leader_path = raw_path # USE leader_path
            
            if self.leader_path: # USE leader_path
                next_point = self.leader_path[0]
                dist = math.sqrt((next_point[0]-self.leader.x)**2 + (next_point[1]-self.leader.y)**2)
                if dist < 40: 
                    self.leader_path.pop(0) # USE leader_path
                if self.leader_path: # USE leader_path
                    leader_goal = self.leader_path[0]
                else:
                    leader_goal = final_destination
            else:
                leader_goal = final_destination

        if leader_goal:
            self.move_robot_dwa(self.leader, leader_goal, walls, is_leader=True)
        
        # ... (rest of the follower and DWA code is the same and correct) ...
        # 2. HISTORY RECORDING & HEADING
        speed = math.sqrt(self.leader.vx**2 + self.leader.vy**2)
        if speed > 0.1 and (not self.leader_history or math.sqrt((self.leader.x - self.leader_history[-1][0])**2 + (self.leader.y - self.leader_history[-1][1])**2) > 5):
            self.leader_history.append((self.leader.x, self.leader.y))

        heading = math.atan2(self.leader.vy, self.leader.vx) if speed > 0.1 else 0

        # 3. FOLLOWER LOGIC (State Machine)
        for i, follower in enumerate(self.followers):
            dist_to_leader = math.sqrt((follower.x - self.leader.x)**2 + (follower.y - self.leader.y)**2)
            
            if dist_to_leader > 200 and self.follower_states[follower] == FollowerState.FORMATION:
                self.follower_states[follower] = FollowerState.REGROUP
                self.follower_paths[follower] = []
                
            elif dist_to_leader < 100 and self.follower_states[follower] == FollowerState.REGROUP:
                self.follower_states[follower] = FollowerState.FORMATION

            if self.follower_states[follower] == FollowerState.REGROUP:
                path = self.follower_paths[follower]
                if not path:
                    raw_path = self.pathfinder.find_path((follower.x, follower.y), (self.leader.x, self.leader.y), walls)
                    if raw_path: self.follower_paths[follower] = raw_path
                
                if self.follower_paths[follower]:
                    next_point = self.follower_paths[follower][0]
                    if math.sqrt((next_point[0]-follower.x)**2 + (next_point[1]-follower.y)**2) < 40:
                        self.follower_paths[follower].pop(0)
                    if self.follower_paths[follower]:
                        self.move_robot_dwa(follower, self.follower_paths[follower][0], walls)
                else:
                    self.move_robot_dwa(follower, (self.leader.x, self.leader.y), walls)
            
            else: # FORMATION mode
                phantom_index = -15 
                phantom_pos = self.leader_history[phantom_index] if len(self.leader_history) >= abs(phantom_index) else (self.leader.x, self.leader.y)

                follower_offsets = [(-40, -50), (-40, 50), (-80, -60), (-80, 60)]
                off_x, off_y = follower_offsets[i]
                
                rot_x = off_x * math.cos(heading) - off_y * math.sin(heading)
                rot_y = off_x * math.sin(heading) + off_y * math.cos(heading)
                
                slot_x, slot_y = phantom_pos[0] + rot_x, phantom_pos[1] + rot_y
                
                self.move_robot_dwa(follower, (slot_x, slot_y), walls)

    def move_robot_dwa(self, robot, goal, walls, is_leader=False):
        # [This function remains unchanged]
        dist_to_goal = math.sqrt((goal[0]-robot.x)**2 + (goal[1]-robot.y)**2)
        speed = math.sqrt(robot.vx**2 + robot.vy**2)
        
        if dist_to_goal > 20 and speed < 0.1: self.stuck_counters[robot] += 1
        else: self.stuck_counters[robot] = max(0, self.stuck_counters[robot] - 2)

        best_vx, best_vy = 0, 0
        max_score = -float('inf')
        
        max_speed = 3.0 if is_leader else 4.0
        dt = 1.2
        separation_dist = 40

        for angle_deg in range(0, 360, 20): 
            angle_rad = math.radians(angle_deg)
            vx = math.cos(angle_rad) * max_speed
            vy = math.sin(angle_rad) * max_speed
            
            pred_x, pred_y = robot.x + vx * dt, robot.y + vy * dt
            
            is_safe = True
            pred_rect = pygame.Rect(pred_x - 15, pred_y - 15, 30, 30)
            
            for wall in walls:
                if wall.rect.colliderect(pred_rect):
                    is_safe = False; break
            
            if is_safe:
                for other in self.all_robots:
                    if other is robot: continue
                    other_pred_x, other_pred_y = other.x + other.vx * dt, other.y + other.vy * dt
                    dist = math.sqrt((pred_x - other_pred_x)**2 + (pred_y - other_pred_y)**2)
                    if dist < separation_dist:
                        is_safe = False; break

            if not is_safe: continue 
                
            gx, gy = goal[0] - robot.x, goal[1] - robot.y
            g_mag = math.sqrt(gx**2 + gy**2)
            heading_score = (vx * gx + vy * gy) / (max_speed * g_mag) if g_mag > 0 else 0
            
            min_wall_dist = 500
            for wall in walls:
                d = math.sqrt((pred_x - wall.rect.centerx)**2 + (pred_y - wall.rect.centery)**2)
                min_wall_dist = min(min_wall_dist, d)
            
            score = (heading_score * 5.0) + (min_wall_dist * 0.05)
            
            if self.stuck_counters.get(robot, 0) > 50:
                 score = (heading_score * 0.1) + (min_wall_dist * 1.0)
                 if self.stuck_counters[robot] > 100: self.stuck_counters[robot] = 0

            if score > max_score:
                max_score = score; best_vx = vx; best_vy = vy

        if max_score > -float('inf'):
            robot.x += best_vx; robot.y += best_vy
            robot.vx = best_vx; robot.vy = best_vy
        else:
            robot.vx, robot.vy = random.uniform(-1, 1), random.uniform(-1, 1)