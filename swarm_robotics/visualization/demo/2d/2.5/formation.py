import math
import pygame
import random
from collections import deque
from pathfinding import PathFinder

class FormationManager:
    def __init__(self, leader, followers, map_width, map_height):
        self.leader = leader
        self.followers = followers
        self.all_robots = [leader] + followers
        
        self.pathfinder = PathFinder(map_width, map_height)
        self.current_path = [] 
        self.path_timer = 0
        
        self.leader_history = deque(maxlen=300) 
        
        self.stuck_counters = {r: 0 for r in self.all_robots}
        
    def update(self, target_obj, walls, user_waypoints):
        # 1. LEADER LOGIC: A* or Waypoint
        leader_goal = None
        
        if len(user_waypoints) > 0:
             next_wp = user_waypoints[0]
             dist_to_wp = math.sqrt((self.leader.x - next_wp[0])**2 + (self.leader.y - next_wp[1])**2)
             if dist_to_wp < 30:
                 user_waypoints.pop(0)
             leader_goal = next_wp
             self.current_path = [] 
        else:
            final_destination = target_obj.rect.center
            self.path_timer += 1
            if self.path_timer > 30 or not self.current_path:
                self.path_timer = 0
                raw_path = self.pathfinder.find_path((self.leader.x, self.leader.y), final_destination, walls)
                if raw_path:
                    self.current_path = raw_path

            if self.current_path:
                next_point = self.current_path[0]
                dist = math.sqrt((next_point[0]-self.leader.x)**2 + (next_point[1]-self.leader.y)**2)
                if dist < 40: 
                    self.current_path.pop(0)
                if self.current_path:
                    leader_goal = self.current_path[0]
                else:
                    leader_goal = final_destination
            else:
                leader_goal = final_destination

        if leader_goal:
            self.move_robot_dwa(self.leader, leader_goal, walls, is_leader=True)
        
        # 2. HISTORY RECORDING & HEADING
        speed = math.sqrt(self.leader.vx**2 + self.leader.vy**2)
        if speed > 0.1:
             if len(self.leader_history) == 0 or \
               math.sqrt((self.leader.x - self.leader_history[-1][0])**2 + (self.leader.y - self.leader_history[-1][1])**2) > 5:
                self.leader_history.append((self.leader.x, self.leader.y))

        heading = 0
        if speed > 0.1: heading = math.atan2(self.leader.vy, self.leader.vx)
        elif len(self.leader_history) > 1: heading = math.atan2(self.leader_history[-1][1] - self.leader_history[-2][1], self.leader_history[-1][0] - self.leader_history[-2][0])

        # 3. FOLLOWER LOGIC (with REGROUP)
        for i, follower in enumerate(self.followers):
            dist_to_leader = math.sqrt((follower.x - self.leader.x)**2 + (follower.y - self.leader.y)**2)
            
            # REGROUP BEHAVIOR: If too far away, forget formation and just go to leader
            if dist_to_leader > 250:
                print(f"[Follower {follower.id}] LOST! Regrouping to Leader.")
                # This follower will now run its own A* to the leader's position
                path_to_leader = self.pathfinder.find_path((follower.x, follower.y), (self.leader.x, self.leader.y), walls)
                if path_to_leader and len(path_to_leader) > 0:
                    self.move_robot_dwa(follower, path_to_leader[0], walls, is_leader=False)
                else: # Failsafe, just DWA to leader
                    self.move_robot_dwa(follower, (self.leader.x, self.leader.y), walls, is_leader=False)
            else:
                # NORMAL FORMATION (Snake Logic)
                phantom_index = -15 
                if len(self.leader_history) >= abs(phantom_index):
                    phantom_pos = self.leader_history[phantom_index]
                else:
                    phantom_pos = (self.leader.x, self.leader.y)

                follower_offsets = [(-40, -50), (-40, 50), (-80, -60), (-80, 60)]

                off_x, off_y = follower_offsets[i]
                rot_x = off_x * math.cos(heading) - off_y * math.sin(heading)
                rot_y = off_x * math.sin(heading) + off_y * math.cos(heading)
                
                slot_x = phantom_pos[0] + rot_x
                slot_y = phantom_pos[1] + rot_y
                
                self.move_robot_dwa(follower, (slot_x, slot_y), walls, is_leader=False)

    def move_robot_dwa(self, robot, goal, walls, is_leader=False):
        
        # Stuck Detection
        dist_to_goal = math.sqrt((goal[0]-robot.x)**2 + (goal[1]-robot.y)**2)
        speed = math.sqrt(robot.vx**2 + robot.vy**2)
        
        if dist_to_goal > 20 and speed < 0.1: self.stuck_counters[robot] += 1
        else: self.stuck_counters[robot] = max(0, self.stuck_counters[robot] - 2)

        # DWA Sampling
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
                
            # Scoring
            gx, gy = goal[0] - robot.x, goal[1] - robot.y
            g_mag = math.sqrt(gx**2 + gy**2)
            heading_score = (vx * gx + vy * gy) / (max_speed * g_mag) if g_mag > 0 else 0
            
            min_wall_dist = 500
            for wall in walls:
                d = math.sqrt((pred_x - wall.rect.centerx)**2 + (pred_y - wall.rect.centery)**2)
                min_wall_dist = min(min_wall_dist, d)
            
            # Higher weight on heading to be more aggressive
            score = (heading_score * 4.0) + (min_wall_dist * 0.05)
            
            if self.stuck_counters[robot] > 50: # If stuck, prioritize escaping
                 score = (heading_score * 0.1) + (min_wall_dist * 1.0)
                 if self.stuck_counters[robot] > 100: self.stuck_counters[robot] = 0 # Reset timer

            if score > max_score:
                max_score = score; best_vx = vx; best_vy = vy

        # Apply
        if max_score > -float('inf'):
            robot.x += best_vx; robot.y += best_vy
            robot.vx = best_vx; robot.vy = best_vy
        else:
            # If completely trapped, wiggle to try and get free
            robot.vx = random.uniform(-1, 1); robot.vy = random.uniform(-1, 1)