import pygame
import socket
import threading
import math
import random

# ====== UDP ======
UDP_IP   = "0.0.0.0"
UDP_PORT = 1234

ESP32_IP   = "192.168.0.12"
ESP32_PORT = 1235

sock_out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

angleX = 0
angleY = 0

def enviar_alerta(codigo):
    try:
        sock_out.sendto(str(codigo).encode(), (ESP32_IP, ESP32_PORT))
    except:
        pass

def recibir_datos():
    global angleX, angleY
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    while True:
        data, _ = sock.recvfrom(1024)
        try:
            valores = data.decode().split(",")
            angleX = -float(valores[0])
            angleY =  float(valores[1])
        except:
            pass

threading.Thread(target=recibir_datos, daemon=True).start()

# ====== PYGAME ======
pygame.init()
WIDTH, HEIGHT = 900, 680
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Simulador de Vuelo — IEX Robótica")
clock      = pygame.time.Clock()
font_hud   = pygame.font.SysFont("Consolas", 14)
font_big   = pygame.font.SysFont("Consolas", 18, bold=True)
font_marca = pygame.font.SysFont("Consolas", 13)

# ====== SONIDO ======
pygame.mixer.init()
pygame.mixer.set_num_channels(8)

sonido_motor  = pygame.mixer.Sound("motor.wav")
sonido_viento = pygame.mixer.Sound("viento.mp3")
sonido_alerta = pygame.mixer.Sound("alarma.mp3")

canal_motor  = pygame.mixer.Channel(0)
canal_viento = pygame.mixer.Channel(1)
canal_alerta = pygame.mixer.Channel(2)

canal_motor.play(sonido_motor, loops=-1)
canal_motor.set_volume(0.4)

alerta_sonando = False
viento_sonando = False

# ====== ESTADO ======
roll_suave   = 0.0
pitch_suave  = 0.0
velocidad    = 0.0
altitud      = 4000.0
grid_offset  = 0.0
tiempo       = 0.0
estado_vuelo = "NIVELADO"

# ====== ESTRELLAS ======
random.seed(99)
STARS = [(random.randint(0, WIDTH), random.randint(20, HEIGHT // 2 + 80),
          random.randint(1, 2), random.randint(160, 255)) for _ in range(180)]

# ====== HELPERS ======
def zona_muerta(valor, umbral=0.1, rampa=0.3):
    if abs(valor) < umbral:
        return 0.0
    t = min(1.0, (abs(valor) - umbral) / rampa)
    return math.copysign(abs(valor) * t, valor)

def rot(cx, cy, x, y, rad):
    dx, dy = x - cx, y - cy
    c, s = math.cos(rad), math.sin(rad)
    return cx + dx*c - dy*s, cy + dx*s + dy*c

def horizon_points(cx, cy, roll_rad, pitch_off):
    L = 4000
    x1 = cx + L * math.cos(roll_rad)
    y1 = cy + L * math.sin(roll_rad) + pitch_off
    x2 = cx - L * math.cos(roll_rad)
    y2 = cy - L * math.sin(roll_rad) + pitch_off
    return (x1, y1), (x2, y2)

# ====== CIELO ======
def draw_sky(surf):
    sky_bands = [
        (0,    (  2,   4,  18)),
        (0.15, (  5,  15,  55)),
        (0.35, ( 15,  55, 140)),
        (0.55, ( 35, 100, 210)),
        (0.75, ( 70, 145, 245)),
        (1.0,  (120, 185, 255)),
    ]
    for i in range(len(sky_bands) - 1):
        t0, c0 = sky_bands[i]
        t1, c1 = sky_bands[i+1]
        y0 = int(t0 * HEIGHT)
        y1 = int(t1 * HEIGHT)
        for y in range(y0, y1):
            t = (y - y0) / max(1, y1 - y0)
            rr = int(c0[0] + (c1[0]-c0[0]) * t)
            gg = int(c0[1] + (c1[1]-c0[1]) * t)
            bb = int(c0[2] + (c1[2]-c0[2]) * t)
            pygame.draw.line(surf, (rr, gg, bb), (0, y), (WIDTH, y))

def draw_stars(surf, cx, cy, roll_rad, pitch_off, h1, h2):
    for sx, sy, size, brightness in STARS:
        rx, ry = sx - cx, sy - cy
        c, s = math.cos(-roll_rad), math.sin(-roll_rad)
        nx = int(rx*c - ry*s + cx)
        ny = int(rx*s + ry*c + cy + pitch_off * 0.25)
        if abs(h2[0] - h1[0]) < 1:
            hor_y = (h1[1] + h2[1]) / 2
        else:
            t = (nx - h1[0]) / (h2[0] - h1[0])
            hor_y = h1[1] + t * (h2[1] - h1[1])
        if ny < hor_y:
            col = (brightness, brightness, min(255, brightness + 20))
            pygame.draw.circle(surf, col, (nx, ny), size)

# ====== TERRENO ======
def draw_ground_and_terrain(surf, cx, cy, roll_rad, pitch_off, grid_offset):
    h1, h2 = horizon_points(cx, cy, roll_rad, pitch_off)

    pygame.draw.polygon(surf, (55, 38, 14), [
        (0, HEIGHT), (WIDTH, HEIGHT),
        (int(h1[0]), int(h1[1])), (int(h2[0]), int(h2[1]))
    ])

    terrain_surf = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)

    FOV      = 300
    GROUND_Y = cy + pitch_off
    ROWS     = 52
    COLS     = 22
    TILE_W   = 60

    for z in range(1, ROWS + 1):
        zf  = z     + grid_offset * 0.09
        znf = z + 1 + grid_offset * 0.09
        if zf <= 0 or znf <= 0:
            continue
        scale  = FOV / zf
        scaleN = FOV / znf
        y_row  = GROUND_Y + scale  * 1.5
        y_next = GROUND_Y + scaleN * 1.5
        if y_next > HEIGHT + 40:
            continue
        if y_row < GROUND_Y - 5:
            continue

        shade = max(20, 138 - z * 2.6)
        fog   = min(1.0, z / 45.0)

        for x in range(-COLS // 2, COLS // 2):
            px1 = cx + (x      * TILE_W) * scale
            px2 = cx + ((x+1)  * TILE_W) * scale
            px3 = cx + ((x+1)  * TILE_W) * scaleN
            px4 = cx + (x      * TILE_W) * scaleN

            even = (int(x + z + grid_offset * 0.05)) % 2 == 0
            if even:
                r = int(shade * 0.55 + fog * 40)
                g = int(shade * 0.45 + fog * 35)
                b = int(shade * 0.18 + fog * 28)
            else:
                r = int(shade * 0.42 + fog * 38)
                g = int(shade * 0.62 + fog * 40)
                b = int(shade * 0.15 + fog * 25)

            r = min(255, r); g = min(255, g); b = min(255, b)
            pts = [
                (int(px1), int(y_row)),
                (int(px2), int(y_row)),
                (int(px3), int(y_next)),
                (int(px4), int(y_next))
            ]
            pygame.draw.polygon(terrain_surf, (r, g, b, 255), pts)
            line_alpha = max(0, int(110 - z * 2.2))
            if line_alpha > 8:
                pygame.draw.polygon(terrain_surf,
                    (min(255,r+30), min(255,g+20), min(255,b+10), line_alpha), pts, 1)

    clip_mask = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    clip_mask.fill((0, 0, 0, 0))
    pygame.draw.polygon(clip_mask, (255, 255, 255, 255), [
        (0, HEIGHT), (WIDTH, HEIGHT),
        (int(h1[0]), int(h1[1])), (int(h2[0]), int(h2[1]))
    ])
    terrain_surf.blit(clip_mask, (0, 0), special_flags=pygame.BLEND_RGBA_MULT)
    surf.blit(terrain_surf, (0, 0))

    pygame.draw.line(surf, (220, 220, 255),
        (int(h1[0]), int(h1[1])), (int(h2[0]), int(h2[1])), 2)

    fog_surf = pygame.Surface((WIDTH, 60), pygame.SRCALPHA)
    for fy in range(60):
        a = int(80 * (1 - fy / 60))
        pygame.draw.line(fog_surf, (180, 200, 230, a), (0, fy), (WIDTH, fy))
    hy_mid = (h1[1] + h2[1]) / 2
    surf.blit(fog_surf, (0, int(hy_mid) - 30))

    return h1, h2

# ====== HUD CENTRAL ======
def draw_hud(surf, cx, cy, roll_rad, pitch_off):
    for deg in [-20, -10, 10, 20]:
        yy = deg * 4.0
        ln = 48 if abs(deg) == 20 else 32
        p1 = rot(cx, cy, cx - ln, cy - yy, roll_rad)
        p2 = rot(cx, cy, cx + ln, cy - yy, roll_rad)
        pygame.draw.line(surf, (0, 210, 0),
            (int(p1[0]), int(p1[1])), (int(p2[0]), int(p2[1])), 1)
        lbl = font_hud.render(str(deg) + "°", True, (0, 200, 0))
        surf.blit(lbl, (int(p2[0]) + 6, int(p2[1]) - 7))

    p1 = rot(cx, cy, cx - 65, cy, roll_rad)
    p2 = rot(cx, cy, cx + 65, cy, roll_rad)
    pygame.draw.line(surf, (0, 255, 0),
        (int(p1[0]), int(p1[1])), (int(p2[0]), int(p2[1])), 1)

    wing = [
        (cx-60, cy), (cx-22, cy), (cx-12, cy+13),
        (cx, cy+13), (cx+12, cy+13), (cx+22, cy),
        (cx+60, cy)
    ]
    for i in range(len(wing)-1):
        pygame.draw.line(surf, (255, 210, 0), wing[i], wing[i+1], 3)
    pygame.draw.line(surf, (255, 210, 0), (cx, cy+13), (cx, cy+19), 3)
    pygame.draw.circle(surf, (0, 255, 0), (cx, cy), 3)

def draw_bank_arc(surf, cx, cy, roll_rad):
    R  = 118
    oy = cy - 25
    pygame.draw.arc(surf, (0, 180, 0),
        (cx-R, oy-R, R*2, R*2),
        math.radians(205), math.radians(335), 1)
    for deg in [-45, -30, -20, -10, 0, 10, 20, 30, 45]:
        a    = math.radians(270 + deg)
        r_in = R - 11
        ix   = cx + r_in * math.cos(a)
        iy   = oy + r_in * math.sin(a)
        ox   = cx + R    * math.cos(a)
        oy2  = oy + R    * math.sin(a)
        w = 2 if deg % 30 == 0 else 1
        pygame.draw.line(surf, (0, 180, 0), (int(ix), int(iy)), (int(ox), int(oy2)), w)

    a_tri = math.radians(270) - roll_rad
    tx    = cx + (R - 6) * math.cos(a_tri)
    ty    = oy + (R - 6) * math.sin(a_tri)
    perp  = a_tri + math.pi / 2
    p1 = (int(tx), int(ty))
    p2 = (int(tx + 9*math.cos(perp)), int(ty + 9*math.sin(perp)))
    p3 = (int(tx - 9*math.cos(perp)), int(ty - 9*math.sin(perp)))
    pygame.draw.polygon(surf, (0, 255, 80), [p1, p2, p3])

# ====== PANEL INFERIOR ======
PANEL_H = 120

def draw_panel(surf, spd, alt, roll, pitch, tiempo, estado):
    py = HEIGHT - PANEL_H
    pygame.draw.rect(surf, (8, 10, 12), (0, py, WIDTH, PANEL_H))
    pygame.draw.line(surf, (0, 160, 0), (0, py), (WIDTH, py), 1)

    bw = 130

    # — Velocidad —
    fp = int(spd / 60 * bw)
    pygame.draw.rect(surf, (0, 90, 0),  (30, py+22, fp, 13))
    pygame.draw.rect(surf, (0, 180, 0), (30, py+22, bw, 13), 1)
    surf.blit(font_hud.render(f"VEL  {int(spd):>3} kt", True, (0, 240, 0)), (30, py+7))

    # — Altitud —
    fa = int(min(max(alt, 0), 5000) / 5000 * bw)
    pygame.draw.rect(surf, (0, 0, 110),  (210, py+22, fa, 13))
    pygame.draw.rect(surf, (0, 180, 0),  (210, py+22, bw, 13), 1)
    surf.blit(font_hud.render(f"ALT  {int(alt):>5} m", True, (0, 240, 0)), (210, py+7))

    # — Ángulos —
    surf.blit(font_hud.render(f"ALABEO   {roll:>+6.1f} deg",  True, (0, 240, 0)), (390, py+7))
    surf.blit(font_hud.render(f"CABECEO  {pitch:>+6.1f} deg", True, (0, 240, 0)), (390, py+27))

    # — Estado de vuelo con histéresis —
    colores = {
        "ASCENSO":  (0, 255, 120),
        "DESCENSO": (255, 140, 0),
        "NIVELADO": (0, 220, 255),
    }
    surf.blit(font_big.render(estado, True, colores[estado]), (670, py+7))

    # — Cronómetro —
    mins = int(tiempo) // 60
    segs = int(tiempo) % 60
    surf.blit(font_hud.render(f"TIEMPO  {mins:02d}:{segs:02d}", True, (0, 180, 0)), (670, py+30))

    # — LED señal UDP —
    blink = (int(tiempo * 2.5) % 2 == 0)
    pygame.draw.circle(surf, (0, 255, 0) if blink else (0, 50, 0), (WIDTH-28, py+28), 7)
    surf.blit(font_hud.render("UDP", True, (0, 160, 0)), (WIDTH-58, py+12))

    # — Separador marca —
    pygame.draw.line(surf, (0, 100, 0), (0, py + PANEL_H - 24), (WIDTH, py + PANEL_H - 24), 1)

    # — IEX | Robótica —
    marca = font_marca.render("IEX  |  Robótica", True, (0, 180, 0))
    surf.blit(marca, (WIDTH // 2 - marca.get_width() // 2, py + PANEL_H - 18))

# ====== LOOP PRINCIPAL ======
running = True
while running:
    dt = clock.tick(60) / 1000.0
    tiempo += dt

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # — Suavizado —
    a_roll  = 0.15
    a_pitch = 0.15
    roll_suave  = a_roll  * angleX + (1 - a_roll)  * roll_suave
    pitch_suave = a_pitch * angleY + (1 - a_pitch) * pitch_suave

    # — Zona muerta suave —
    roll_suave  = zona_muerta(roll_suave)
    pitch_suave = zona_muerta(pitch_suave)

    # — Física —
    velocidad += pitch_suave * 0.012
    velocidad  = max(0.0, min(60.0, velocidad))
    altitud   += pitch_suave * 0.5
    grid_offset = (grid_offset + velocidad * 0.5) % 100

    # — Histéresis estado de vuelo —
    if estado_vuelo == "NIVELADO":
        if pitch_suave > 3.0:
            estado_vuelo = "ASCENSO"
        elif pitch_suave < -3.0:
            estado_vuelo = "DESCENSO"
    elif estado_vuelo == "ASCENSO":
        if pitch_suave < 1.0:
            estado_vuelo = "NIVELADO"
    elif estado_vuelo == "DESCENSO":
        if pitch_suave > -1.0:
            estado_vuelo = "NIVELADO"

    # — Sonidos —
    vol_motor = 0.2 + (velocidad / 60) * 0.7
    canal_motor.set_volume(vol_motor)

    en_descenso_rapido = pitch_suave < -15 or velocidad > 45
    if en_descenso_rapido and not viento_sonando:
        canal_viento.play(sonido_viento, loops=-1)
        viento_sonando = True
    elif not en_descenso_rapido and viento_sonando:
        canal_viento.stop()
        viento_sonando = False
    if viento_sonando:
        vol_viento = min(1.0, (velocidad - 30) / 30 + abs(pitch_suave) / 30)
        canal_viento.set_volume(max(0, vol_viento))

    en_peligro = altitud < 2000 or (velocidad < 5 and pitch_suave < -10)
    if en_peligro and not alerta_sonando:
        canal_alerta.play(sonido_alerta, loops=-1)
        enviar_alerta(1)
        alerta_sonando = True
    elif not en_peligro and alerta_sonando:
        canal_alerta.stop()
        enviar_alerta(0)
        alerta_sonando = False

    # — Render —
    cx = WIDTH  // 2
    cy = (HEIGHT - PANEL_H) // 2
    roll_rad  = math.radians(roll_suave)
    pitch_off = pitch_suave * 2.5

    draw_sky(screen)
    h1, h2 = draw_ground_and_terrain(screen, cx, cy, roll_rad, pitch_off, grid_offset)
    draw_stars(screen, cx, cy, roll_rad, pitch_off, h1, h2)
    draw_hud(screen, cx, cy, roll_rad, pitch_off)
    draw_bank_arc(screen, cx, cy, roll_rad)
    draw_panel(screen, velocidad, altitud, roll_suave, pitch_suave, tiempo, estado_vuelo)

    pygame.draw.rect(screen, (0, 180, 0), (0, 0, WIDTH, HEIGHT), 2)
    pygame.display.flip()

pygame.quit()