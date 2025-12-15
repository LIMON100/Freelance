# communication.py
import pygame
import math

class CommunicationVisualizer:
    """
    A class to visualize the communication network between robots.
    It draws lines between robots that are within a certain communication range.
    The color of the line indicates the signal strength (stronger signal = brighter line).
    """
    def __init__(self, robots, comm_range=200, line_color=(100, 100, 255), strong_color=(200, 200, 255)):
        """
        Initializes the CommunicationVisualizer.
        
        Args:
            robots (list): A list of Robot objects. The visualizer will access the .x and .y attributes of these robots.
            comm_range (int): The maximum distance at which two robots can communicate.
            line_color (tuple): The RGB color for the line when the signal is weakest (at max range).
            strong_color (tuple): The RGB color for the line when the signal is strongest (at zero distance).
        """
        self.robots = robots
        self.comm_range = comm_range
        self.line_color = line_color
        self.strong_color = strong_color

    def draw(self, surface):
        """
        Draws the communication links between robots on the given pygame surface.
        
        Args:
            surface (pygame.Surface): The surface on which to draw the communication lines.
        """
        num_robots = len(self.robots)
        
        # Iterate through all unique pairs of robots
        for i in range(num_robots):
            for j in range(i + 1, num_robots):
                robot1 = self.robots[i]
                robot2 = self.robots[j]
                
                # Calculate the distance between the two robots
                dx = robot1.x - robot2.x
                dy = robot1.y - robot2.y
                distance = math.sqrt(dx**2 + dy**2)
                
                # If they are within communication range, draw a line
                if distance <= self.comm_range:
                    # Calculate signal strength (0.0 to 1.0)
                    # A distance of 0 is a strength of 1.0 (strongest)
                    # A distance of comm_range is a strength of 0.0 (weakest)
                    if self.comm_range > 0:
                        strength = 1.0 - (distance / self.comm_range)
                    else:
                        strength = 1.0 # Avoid division by zero if range is 0
                    
                    # Interpolate color based on signal strength
                    color = [
                        int(self.strong_color[k] * strength + self.line_color[k] * (1 - strength))
                        for k in range(3)
                    ]
                    
                    # Draw the communication line
                    pygame.draw.line(surface, color, (robot1.x, robot1.y), (robot2.x, robot2.y), 2)
