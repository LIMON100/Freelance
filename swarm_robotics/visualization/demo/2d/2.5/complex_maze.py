import pygame
import sys
from formation import FormationManager
from communication import CommunicationVisualizer
from pathfinding import PathFinder

# --- Constants ---
MAP_WIDTH = 1280
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
COLOR_WAYPOINT = (255, 255, 0)
COLOR_BTN_START = (0, 200, 0)
COLOR_BTN_STOP = (200, 0, 0)
COLOR_BTN_RESET = (100, 100, 100)
COLOR_BTN_REMOVE = (200, 100, 50)

# --- Classes --- (Button, WaypointManager, Target, Robot, Wall - all same as before)
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
        self.waypoints = []

    def add_point(self, pos):
        if pos[0] < MAP_WIDTH:
            self.waypoints.append(pos)

    def remove_last(self):
        if self.waypoints:
            self.waypoints.pop()

    def clear(self):
        self.waypoints = []

    def draw(self, surface):
        if len(self.waypoints) > 1:
            pygame.draw.lines(surface, COLOR_WAYPOINT, False, self.waypoints, 2)
        for i, pt in enumerate(self.waypoints):
            pygame.draw.circle(surface, COLOR_WAYPOINT, pt, WAYPOINT_RADIUS)
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


class LeaderSelectButton:
    def __init__(self, x, y, robot_index, label):
        self.rect = pygame.Rect(x, y, 40, 40)
        self.robot_index = robot_index
        self.label = label
        self.color_off = (100, 100, 200) # Dim Blue
        self.color_on = (255, 50, 50)    # Red (Active Leader)

    def draw(self, surface, font, current_leader_index):
        # Check if this button represents the current leader
        is_active = (self.robot_index == current_leader_index)
        color = self.color_on if is_active else self.color_off
        
        # Draw Circle Button
        pygame.draw.circle(surface, color, self.rect.center, 18)
        pygame.draw.circle(surface, (255, 255, 255), self.rect.center, 18, 2)
        
        text_surf = font.render(self.label, True, (255, 255, 255))
        text_rect = text_surf.get_rect(center=self.rect.center)
        surface.blit(text_surf, text_rect)

    def is_clicked(self, pos):
        # Simple rect check for click
        return self.rect.collidepoint(pos)

# --- Main Application ---
# --- Main Application ---
def main():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Swarm Robotics: MAZE CHALLENGE")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("Consolas", 16)

    sim_running = False

    target = Target(100, 100)
    
    # Robots start at the opposite corner
    robots = [
        Robot("F0", 1150, 800, is_leader=True),
        Robot("F1", 1100, 850),
        Robot("F2", 1200, 850),
        Robot("F3", 1050, 900),
        Robot("F4", 1250, 900) 
    ]

    walls = [
        # Outer Boundary
        Wall(0, 0, MAP_WIDTH, 20),           # Top
        Wall(0, SCREEN_HEIGHT - 20, MAP_WIDTH, 20), # Bottom
        Wall(0, 0, 20, SCREEN_HEIGHT),       # Left
        Wall(MAP_WIDTH - 20, 0, 20, SCREEN_HEIGHT), # Right

        # --- Maze Interior Walls ---
        Wall(200, 20, 20, 400),
        Wall(200, 400, 300, 20),
        Wall(500, 400, 20, 400),
        Wall(500, 800, 300, 20),
        Wall(800, 500, 20, 300),
        Wall(800, 500, 300, 20),
        Wall(20, 200, 100, 20),
        Wall(350, 20, 20, 200),
        Wall(600, 20, 20, 150),
        Wall(700, 150, 150, 20),
        Wall(850, 20, 20, 150),
        Wall(20, 600, 400, 20),
        Wall(150, 600, 20, 200),
        Wall(300, 750, 220, 20),
        Wall(950, 960, 20, -300), 
        Wall(950, 660, 200, 20),
        Wall(400, 550, 50, 50),
        Wall(700, 300, 50, 50),
    ]

    waypoint_mgr = WaypointManager()
    
    # 1. Standard Buttons
    btn_start = Button(MAP_WIDTH + 20, 20, 120, 40, "START", COLOR_BTN_START)
    btn_reset = Button(MAP_WIDTH + 160, 20, 120, 40, "RESET", COLOR_BTN_RESET)
    
    btn_clear_pts = Button(MAP_WIDTH + 20, 300, 120, 30, "CLEAR PTS", COLOR_BTN_REMOVE)
    btn_undo_pt = Button(MAP_WIDTH + 160, 300, 120, 30, "UNDO PT", COLOR_BTN_REMOVE)

    buttons = [btn_start, btn_reset, btn_clear_pts, btn_undo_pt]

    # 2. Leader Selection Buttons
    leader_buttons = []
    start_x = MAP_WIDTH + 30
    start_y = 550 # Position below telemetry
    labels = ["F0", "F1", "F2", "F3", "F4"]
    
    for i in range(5):
        # Create circle buttons
        btn = LeaderSelectButton(start_x + (i * 50), start_y, i, labels[i])
        leader_buttons.append(btn)

    # Logic Managers
    leader_robot = robots[0]
    follower_robots = robots[1:]
    
    formation_mgr = FormationManager(leader_robot, follower_robots, MAP_WIDTH, SCREEN_HEIGHT)
    comm_viz = CommunicationVisualizer(robots)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            
            if event.type == pygame.MOUSEBUTTONDOWN:
                if btn_start.is_clicked(event.pos):
                    sim_running = not sim_running
                    # Update button text/color immediately
                    if sim_running:
                        btn_start.text = "STOP"
                        btn_start.color = COLOR_BTN_STOP
                    else:
                        btn_start.text = "START"
                        btn_start.color = COLOR_BTN_START
                
                elif btn_reset.is_clicked(event.pos):
                    sim_running = False
                    btn_start.text = "START"
                    btn_start.color = COLOR_BTN_START
                    target.reset()
                    for r in robots: r.reset()
                    waypoint_mgr.clear()
                    formation_mgr.leader_history.clear()
                    # Reset leader to default (Robot 0)
                    formation_mgr.switch_leader(0)
                
                elif btn_clear_pts.is_clicked(event.pos):
                    if not sim_running: waypoint_mgr.clear()
                elif btn_undo_pt.is_clicked(event.pos):
                    if not sim_running: waypoint_mgr.remove_last()
                
                else:
                    # Check Leader Buttons
                    leader_clicked = False
                    for l_btn in leader_buttons:
                        if l_btn.is_clicked(event.pos):
                            formation_mgr.switch_leader(l_btn.robot_index)
                            leader_clicked = True
                            break # Stop checking other leader buttons

                    # Check Map Click (only if didn't click a leader button)
                    if not leader_clicked and event.pos[0] < MAP_WIDTH and not sim_running:
                        clicked_obj = False
                        if target.rect.collidepoint(event.pos): clicked_obj = True
                        for w in walls: 
                            if w.rect.collidepoint(event.pos): clicked_obj = True
                        
                        if not clicked_obj:
                            waypoint_mgr.add_point(event.pos)

            if not target.handle_event(event):
                pass

        if sim_running:
            formation_mgr.update(target, walls, waypoint_mgr.waypoints)

        # Drawing
        screen.fill(COLOR_BG)
        target.draw(screen)
        for w in walls: w.draw(screen)
        waypoint_mgr.draw(screen)
        
        # --- FIX IS HERE ---
        # We check for 'leader_path' instead of 'current_path'
        if sim_running and hasattr(formation_mgr, 'leader_path') and len(formation_mgr.leader_path) > 1:
             pygame.draw.lines(screen, (255, 255, 0), False, formation_mgr.leader_path, 2)
        # Fallback if you are using an older formation.py
        elif sim_running and hasattr(formation_mgr, 'current_path') and len(formation_mgr.current_path) > 1:
             pygame.draw.lines(screen, (255, 255, 0), False, formation_mgr.current_path, 2)
        
        comm_viz.draw(screen)
        for r in robots: r.draw(screen)

        # UI Panel
        pygame.draw.rect(screen, COLOR_PANEL, (MAP_WIDTH, 0, PANEL_WIDTH, SCREEN_HEIGHT))
        pygame.draw.line(screen, (100, 100, 100), (MAP_WIDTH, 0), (MAP_WIDTH, SCREEN_HEIGHT), 2)
        
        # Ensure button text is up to date in drawing loop
        btn_start.text = "STOP" if sim_running else "START"
        btn_start.color = COLOR_BTN_STOP if sim_running else COLOR_BTN_START
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
            info = f"ID: {r.id:2} | Pos: ({int(r.x):04}, {int(r.y):03})"
            screen.blit(font.render(info, True, color), (MAP_WIDTH + 20, y_offset))
            y_offset += 25

        # Draw Leader Selectors
        header_leader = font.render("SELECT LEADER:", True, (200, 200, 200))
        screen.blit(header_leader, (MAP_WIDTH + 20, 520))
        
        # Get Current Leader Index
        curr_leader_idx = formation_mgr.all_robots.index(formation_mgr.leader)
        
        for l_btn in leader_buttons:
            l_btn.draw(screen, font, curr_leader_idx)

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()