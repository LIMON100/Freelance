# In lidar.py
import math
import pygame

class Lidar:
    def __init__(self, owner, max_range=400, num_rays=90):
        self.owner = owner # The robot this lidar is attached to
        self.max_range = max_range
        self.num_rays = num_rays
        self.angle_increment = 360 / num_rays
        
        # This will store the points where the rays hit something
        self.scan_points = []

    def update(self, walls):
        self.scan_points = []
        
        for i in range(self.num_rays):
            angle = math.radians(i * self.angle_increment)
            
            # End point of the ray if it hits nothing
            end_x = self.owner.x + self.max_range * math.cos(angle)
            end_y = self.owner.y + self.max_range * math.sin(angle)
            
            closest_dist = self.max_range
            closest_point = (end_x, end_y)

            # Check intersection with every wall
            for wall in walls:
                hit = wall.rect.clipline((self.owner.x, self.owner.y), (end_x, end_y))
                
                if hit:
                    # If it hits, 'hit' is a tuple of (start, end) points of the clipped line
                    hit_point = hit[0] # The point where the ray enters the rect
                    dist = math.sqrt((hit_point[0] - self.owner.x)**2 + (hit_point[1] - self.owner.y)**2)
                    
                    if dist < closest_dist:
                        closest_dist = dist
                        closest_point = hit_point

            self.scan_points.append(closest_point)

    def draw(self, surface):
        # Draw the Lidar rays (optional, but great for debugging)
        for point in self.scan_points:
            pygame.draw.line(surface, (255, 0, 255, 50), (self.owner.x, self.owner.y), point, 1)
        
        # Draw the hit points
        for point in self.scan_points:
            pygame.draw.circle(surface, (255, 100, 0), (int(point[0]), int(point[1])), 2)