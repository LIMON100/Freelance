import pygame
import sys
from formation import FormationManager
from communication import CommunicationVisualizer

# --- Constants ---
MAP_WIDTH = 1280
PANEL_WIDTH = 300  # Extra space for the UI
SCREEN_WIDTH = MAP_WIDTH + PANEL_WIDTH
SCREEN_HEIGHT = 720

ROBOT_RADIUS = 15
TARGET_SIZE = 40

# Colors (R, G, B)
COLOR_BG = (30, 30, 30)         # Map Background
COLOR_PANEL = (50, 50, 50)      # UI Panel Background
COLOR_TEXT = (255, 255, 255)
COLOR_LEADER = (255, 50, 50)    # Red
COLOR_FOLLOWER = (50, 100, 255) # Blue
COLOR_WALL = (255, 255, 255)    # White
COLOR_WALL_ACTIVE = (200, 200, 200)
COLOR_TARGET = (0, 255, 0)      # BIG GREEN
COLOR_BTN_START = (0, 200, 0)
COLOR_BTN_STOP = (200, 0, 0)
COLOR_BTN_RESET = (100, 100, 100)

# --- Classes ---

class Button:
    def __init__(self, x, y, w, h, text, color, action_code):
        self.rect = pygame.Rect(x, y, w, h)
        self.text = text
        self.color = color
        self.action_code = action_code # 0=Start/Stop, 1=Reset

    def draw(self, surface, font):
        pygame.draw.rect(surface, self.color, self.rect)
        pygame.draw.rect(surface, (255,255,255), self.rect, 2) # Border
        text_surf = font.render(self.text, True, (255, 255, 255))
        text_rect = text_surf.get_rect(center=self.rect.center)
        surface.blit(text_surf, text_rect)

    def is_clicked(self, pos):
        return self.rect.collidepoint(pos)

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
        # Draw "T" for Target
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
                # Keep target inside Map area
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

        # NEW: Velocity tracking
        self.vx = 0
        self.vy = 0

    def draw(self, surface):
        pygame.draw.circle(surface, self.color, (int(self.x), int(self.y)), ROBOT_RADIUS)
        pygame.draw.circle(surface, (0, 0, 0), (int(self.x), int(self.y)), ROBOT_RADIUS, 2)
        
        # Draw ID
        font = pygame.font.SysFont("Arial", 12, bold=True)
        text = font.render(str(self.id), True, (255, 255, 255))
        surface.blit(text, (int(self.x) - 4, int(self.y) - 7))

    def reset(self):
        self.x, self.y = self.start_pos

    # Placeholder for movement logic (we will add DWA here later)
    def update(self):
        pass 

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
    font = pygame.font.SysFont("Consolas", 16) # Monospaced font for coordinates

    # 1. Init Simulation State
    sim_running = False

    # 2. Init Objects
    target = Target(100, 100)
    
    robots = [
        Robot("L", 600, 600, is_leader=True),
        Robot("F1", 550, 650),
        Robot("F2", 650, 650),
        Robot("F3", 500, 700),
        Robot("F4", 700, 700)
    ]
    
    walls = [
        Wall(200, 300, 30, 200),
        Wall(600, 200, 300, 30),
        Wall(900, 400, 30, 200)
    ]

    # 3. Init Buttons
    btn_start = Button(MAP_WIDTH + 20, 20, 100, 40, "START", COLOR_BTN_START, 0)
    btn_reset = Button(MAP_WIDTH + 140, 20, 100, 40, "RESET", COLOR_BTN_RESET, 1)
    buttons = [btn_start, btn_reset]

    # comm_viz = CommunicationVisualizer(robots, comm_range=250)

    # Separate leader and followers for the manager
    leader_robot = robots[0]
    follower_robots = robots[1:] # All robots except the first one

    # Initialize Logic Modules
    formation_mgr = FormationManager(leader_robot, follower_robots)
    comm_viz = CommunicationVisualizer(robots)

    # --- Main Loop ---
    running = True
    while running:
        # --- Event Handling ---
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            
            # Handle Button Clicks
            if event.type == pygame.MOUSEBUTTONDOWN:
                if btn_start.is_clicked(event.pos):
                    sim_running = not sim_running
                    # Toggle Button text/color
                    if sim_running:
                        btn_start.text = "STOP"
                        btn_start.color = COLOR_BTN_STOP
                    else:
                        btn_start.text = "START"
                        btn_start.color = COLOR_BTN_START
                
                if btn_reset.is_clicked(event.pos):
                    sim_running = False
                    btn_start.text = "START"
                    btn_start.color = COLOR_BTN_START
                    # Reset all objects
                    target.reset()
                    for r in robots: r.reset()
                    for w in walls: w.reset()

            # Handle Draggables (Target & Walls)
            # Only allow dragging if simulation is STOPPED (optional, but cleaner)
            if not target.handle_event(event):
                for wall in reversed(walls):
                    if wall.handle_event(event):
                        break

        # --- Update Logic ---
        # if sim_running:
        #     for r in robots:
        #         r.update()

        if sim_running:
        # Only one line needed now!
        # The manager handles moving ALL robots (Leader + Followers)
            formation_mgr.update(target, walls)

        # --- Drawing ---
        screen.fill(COLOR_BG)

        # Draw Map Objects
        target.draw(screen)
        for w in walls: w.draw(screen)

        # comm_viz.draw(screen) 

        for r in robots: r.draw(screen)

        # Draw Right Side Panel
        pygame.draw.rect(screen, COLOR_PANEL, (MAP_WIDTH, 0, PANEL_WIDTH, SCREEN_HEIGHT))
        pygame.draw.line(screen, (100, 100, 100), (MAP_WIDTH, 0), (MAP_WIDTH, SCREEN_HEIGHT), 2)

        # Draw Buttons
        for btn in buttons:
            btn.draw(screen, font)

        # Draw Telemetry Text
        # 1. Target Info
        target_text = font.render(f"TARGET: ({target.rect.x}, {target.rect.y})", True, COLOR_TARGET)
        screen.blit(target_text, (MAP_WIDTH + 20, 80))

        # 2. Robot Info
        y_offset = 120
        header = font.render(f"ROBOT TELEMETRY:", True, (200, 200, 200))
        screen.blit(header, (MAP_WIDTH + 20, y_offset))
        y_offset += 30

        for r in robots:
            # Color code the text based on robot role
            color = COLOR_LEADER if r.is_leader else COLOR_FOLLOWER
            info = f"ID: {r.id:2} | Pos: ({int(r.x):03}, {int(r.y):03})"
            text = font.render(info, True, color)
            screen.blit(text, (MAP_WIDTH + 20, y_offset))
            y_offset += 25

        # 3. Status Info
        status_color = (0, 255, 0) if sim_running else (255, 100, 0)
        status_txt = "RUNNING" if sim_running else "PAUSED"
        status_surf = font.render(f"STATUS: {status_txt}", True, status_color)
        screen.blit(status_surf, (MAP_WIDTH + 20, SCREEN_HEIGHT - 40))

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()