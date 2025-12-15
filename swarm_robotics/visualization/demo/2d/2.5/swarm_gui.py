import pygame
import sys
from formation import FormationManager
from communication import CommunicationVisualizer
from pathfinding import PathFinder

# --- Constants ---
MAP_WIDTH = 1300
PANEL_WIDTH = 300
SCREEN_WIDTH = MAP_WIDTH + PANEL_WIDTH
SCREEN_HEIGHT = 980

ROBOT_RADIUS = 15
TARGET_SIZE = 40
WAYPOINT_RADIUS = 8

# Colors
COLOR_BG = (30, 30, 30)
COLOR_PANEL = (50, 50, 50)
COLOR_TEXT = (255, 255, 255)
COLOR_LEADER = (255, 50, 50)
COLOR_FOLLOWER = (50, 100, 255)
COLOR_WALL = (255, 255, 255)
COLOR_WALL_ACTIVE = (200, 200, 200)
COLOR_TARGET = (0, 255, 0)
COLOR_WAYPOINT = (255, 255, 0) # Yellow for waypoints
COLOR_BTN_START = (0, 200, 0)
COLOR_BTN_STOP = (200, 0, 0)
COLOR_BTN_RESET = (100, 100, 100)
COLOR_BTN_REMOVE = (200, 100, 50) # Orange for remove

# --- Classes ---

class Button:
    def __init__(self, x, y, w, h, text, color):
        self.rect = pygame.Rect(x, y, w, h)
        self.text = text
        self.color = color

    def draw(self, surface, font):
        pygame.draw.rect(surface, self.color, self.rect)
        pygame.draw.rect(surface, (255,255,255), self.rect, 2)
        text_surf = font.render(self.text, True, (255, 255, 255))
        text_rect = text_surf.get_rect(center=self.rect.center)
        surface.blit(text_surf, text_rect)

    def is_clicked(self, pos):
        return self.rect.collidepoint(pos)

class WaypointManager:
    def __init__(self):
        self.waypoints = [] # List of (x, y) tuples

    def add_point(self, pos):
        # Only add if inside map area
        if pos[0] < MAP_WIDTH:
            self.waypoints.append(pos)

    def remove_last(self):
        if self.waypoints:
            self.waypoints.pop()

    def clear(self):
        self.waypoints = []

    def draw(self, surface):
        if len(self.waypoints) > 0:
            # Draw lines connecting waypoints
            if len(self.waypoints) > 1:
                pygame.draw.lines(surface, COLOR_WAYPOINT, False, self.waypoints, 2)
            
            # Draw dots
            for i, pt in enumerate(self.waypoints):
                pygame.draw.circle(surface, COLOR_WAYPOINT, pt, WAYPOINT_RADIUS)
                # Draw number
                font = pygame.font.SysFont("Arial", 12)
                text = font.render(str(i+1), True, (0,0,0))
                surface.blit(text, (pt[0]-4, pt[1]-8))

class Target:
    def __init__(self, x, y):
        self.rect = pygame.Rect(x, y, TARGET_SIZE, TARGET_SIZE)
        self.start_pos = (x, y)
        self.color = COLOR_TARGET
        self.is_dragging = False
        self.offset_x = 0
        self.offset_y = 0

    def draw(self, surface):
        pygame.draw.rect(surface, self.color, self.rect)
        font = pygame.font.SysFont("Arial", 20, bold=True)
        text = font.render("T", True, (0,0,0))
        surface.blit(text, (self.rect.x + 12, self.rect.y + 8))

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1 and self.rect.collidepoint(event.pos):
                self.is_dragging = True
                self.offset_x = self.rect.x - event.pos[0]
                self.offset_y = self.rect.y - event.pos[1]
                return True
        elif event.type == pygame.MOUSEBUTTONUP:
            self.is_dragging = False
        elif event.type == pygame.MOUSEMOTION:
            if self.is_dragging:
                self.rect.x = event.pos[0] + self.offset_x
                self.rect.y = event.pos[1] + self.offset_y
                self.rect.x = max(0, min(self.rect.x, MAP_WIDTH - TARGET_SIZE))
                self.rect.y = max(0, min(self.rect.y, SCREEN_HEIGHT - TARGET_SIZE))
                return True
        return False
    
    def reset(self):
        self.rect.x, self.rect.y = self.start_pos

class Robot:
    def __init__(self, id, x, y, is_leader=False):
        self.id = id
        self.x = x
        self.y = y
        self.start_pos = (x, y)
        self.is_leader = is_leader
        self.color = COLOR_LEADER if is_leader else COLOR_FOLLOWER
        self.vx = 0
        self.vy = 0

    def draw(self, surface):
        pygame.draw.circle(surface, self.color, (int(self.x), int(self.y)), ROBOT_RADIUS)
        pygame.draw.circle(surface, (0, 0, 0), (int(self.x), int(self.y)), ROBOT_RADIUS, 2)
        font = pygame.font.SysFont("Arial", 12, bold=True)
        text = font.render(str(self.id), True, (255, 255, 255))
        surface.blit(text, (int(self.x) - 4, int(self.y) - 7))

    def reset(self):
        self.x, self.y = self.start_pos
        self.vx = 0
        self.vy = 0

class Wall:
    def __init__(self, x, y, w, h):
        self.rect = pygame.Rect(x, y, w, h)
        self.start_rect = pygame.Rect(x, y, w, h)
        self.color = COLOR_WALL
        self.is_dragging = False
        self.offset_x = 0
        self.offset_y = 0

    def draw(self, surface):
        color = COLOR_WALL_ACTIVE if self.is_dragging else self.color
        pygame.draw.rect(surface, color, self.rect)

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1 and self.rect.collidepoint(event.pos):
                self.is_dragging = True
                self.offset_x = self.rect.x - event.pos[0]
                self.offset_y = self.rect.y - event.pos[1]
                return True
        elif event.type == pygame.MOUSEBUTTONUP:
            self.is_dragging = False
        elif event.type == pygame.MOUSEMOTION:
            if self.is_dragging:
                self.rect.x = event.pos[0] + self.offset_x
                self.rect.y = event.pos[1] + self.offset_y
                return True
        return False

    def reset(self):
        self.rect.x = self.start_rect.x
        self.rect.y = self.start_rect.y

# --- Main Application ---
def main():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Swarm Robotics Control Station")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("Consolas", 16)

    # 1. State
    sim_running = False

    # 2. Objects
    target = Target(100, 100)
    waypoint_mgr = WaypointManager()
    
    robots = [
        Robot("L", 600, 600, is_leader=True),
        Robot("F1", 550, 650),
        Robot("F2", 650, 650),
        Robot("F3", 500, 700),
        Robot("F4", 700, 700)
    ]
    
    walls = [
        Wall(200, 300, 30, 200), Wall(600, 200, 300, 30),
        Wall(800, 500, 30, 150), Wall(400, 550, 150, 30),
        Wall(100, 100, 100, 100), Wall(950, 150, 30, 300),
        Wall(300, 100, 30, 100), Wall(700, 600, 100, 30)
    ]

    # 3. Buttons
    btn_start = Button(MAP_WIDTH + 20, 20, 120, 40, "START", COLOR_BTN_START)
    btn_reset = Button(MAP_WIDTH + 160, 20, 120, 40, "RESET", COLOR_BTN_RESET)
    
    btn_clear_pts = Button(MAP_WIDTH + 20, 300, 120, 30, "CLEAR PTS", COLOR_BTN_REMOVE)
    btn_undo_pt = Button(MAP_WIDTH + 160, 300, 120, 30, "UNDO PT", COLOR_BTN_REMOVE)
    
    buttons = [btn_start, btn_reset, btn_clear_pts, btn_undo_pt]

    leader_robot = robots[0]
    follower_robots = robots[1:]
    
    # Initialize Manager with Waypoint support
    formation_mgr = FormationManager(leader_robot, follower_robots, MAP_WIDTH, SCREEN_HEIGHT)
    comm_viz = CommunicationVisualizer(robots)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            
            if event.type == pygame.MOUSEBUTTONDOWN:
                # Handle Buttons
                if btn_start.is_clicked(event.pos):
                    sim_running = not sim_running
                    btn_start.text = "STOP" if sim_running else "START"
                    btn_start.color = COLOR_BTN_STOP if sim_running else COLOR_BTN_START
                
                elif btn_reset.is_clicked(event.pos):
                    sim_running = False
                    btn_start.text = "START"
                    btn_start.color = COLOR_BTN_START
                    target.reset()
                    for r in robots: r.reset()
                    for w in walls: w.reset()
                    waypoint_mgr.clear() # Clear waypoints on reset
                    formation_mgr.leader_history.clear() # Clear snake trail
                
                elif btn_clear_pts.is_clicked(event.pos):
                    if not sim_running: waypoint_mgr.clear()

                elif btn_undo_pt.is_clicked(event.pos):
                    if not sim_running: waypoint_mgr.remove_last()
                
                # Handle Map Click (Add Waypoint)
                # Only if NOT clicking a button, NOT dragging target/wall, and simulation PAUSED
                elif event.pos[0] < MAP_WIDTH and not sim_running:
                    # Check if clicked on wall or target (don't add waypoint then)
                    clicked_obj = False
                    if target.rect.collidepoint(event.pos): clicked_obj = True
                    for w in walls: 
                        if w.rect.collidepoint(event.pos): clicked_obj = True
                    
                    if not clicked_obj:
                        waypoint_mgr.add_point(event.pos)

            # Handle Draggables
            if not target.handle_event(event):
                for wall in reversed(walls):
                    if wall.handle_event(event): break

        # Logic Update
        if sim_running:
            # Pass waypoints to formation manager
            # If waypoints exist, they override direct path to target
            formation_mgr.update(target, walls, waypoint_mgr.waypoints)

        # Drawing
        screen.fill(COLOR_BG)
        
        # Draw Objects
        target.draw(screen)
        for w in walls: w.draw(screen)
        waypoint_mgr.draw(screen) # Draw yellow line/dots
        
        # Draw A* Path (Yellow Line) - Visualization
        if sim_running and len(formation_mgr.current_path) > 1:
             pygame.draw.lines(screen, (255, 255, 0), False, formation_mgr.current_path, 2)
        
        comm_viz.draw(screen)
        for r in robots: r.draw(screen)

        # Draw UI
        pygame.draw.rect(screen, COLOR_PANEL, (MAP_WIDTH, 0, PANEL_WIDTH, SCREEN_HEIGHT))
        pygame.draw.line(screen, (100, 100, 100), (MAP_WIDTH, 0), (MAP_WIDTH, SCREEN_HEIGHT), 2)
        
        for btn in buttons: btn.draw(screen, font)

        # Telemetry
        target_text = font.render(f"TARGET: ({target.rect.centerx}, {target.rect.centery})", True, COLOR_TARGET)
        screen.blit(target_text, (MAP_WIDTH + 20, 80))
        
        pts_text = font.render(f"WAYPOINTS: {len(waypoint_mgr.waypoints)}", True, COLOR_WAYPOINT)
        screen.blit(pts_text, (MAP_WIDTH + 20, 110))

        y_offset = 350
        screen.blit(font.render(f"ROBOT TELEMETRY:", True, (200, 200, 200)), (MAP_WIDTH + 20, y_offset))
        y_offset += 30
        for r in robots:
            color = COLOR_LEADER if r.is_leader else COLOR_FOLLOWER
            info = f"ID: {r.id:2} | Pos: ({int(r.x):03}, {int(r.y):03})"
            screen.blit(font.render(info, True, color), (MAP_WIDTH + 20, y_offset))
            y_offset += 25

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()