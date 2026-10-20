import orx
import pyscroll

# MagiaSmash game director.
#
# Owns the match flow: scene creation, wave spawning, restarts.
# The C++ side (Fighter/GameHUD) publishes shared state in the
# [MatchState] config section, which is polled every frame here.

MAX_WAVE       = 5
CLEAR_DELAY    = 1.2
RESPAWN_DELAY  = 0.4

g_player  = None
g_enemies = []
g_wave    = 0
g_timer   = 0.0

# [MatchState] helpers ---------------------------------------------------------
#
# NB: pocketpy does NOT unwind `with` blocks on `return` (TK_RETURN emits
# OP_RETURN_VALUE without popping enclosing CodeBlockType_WITH blocks), so
# a `return` inside a `with orx.config.Section(...)` leaks a config stack
# push and eventually overflows the 64-entry engine stack. Always assign
# inside the block and return after it.

def set_state_string(key: str, value: str):
  with orx.config.Section("MatchState"):
    orx.config.set_string(key, value)

def set_state_int(key: str, value: int):
  with orx.config.Section("MatchState"):
    orx.config.set_int(key, value)

def set_state_float(key: str, value: float):
  with orx.config.Section("MatchState"):
    orx.config.set_float(key, value)

def get_state_int(key: str) -> int:
  # NB: never `return` from inside a `with` block in pocketpy - it does not
  # unwind context managers on return, so the config section would stay pushed.
  with orx.config.Section("MatchState"):
    value = orx.config.get_int(key)
  return value

def get_state_string(key: str) -> str:
  with orx.config.Section("MatchState"):
    value = orx.config.get_string(key)
  return value

# Match flow -------------------------------------------------------------------

def spawn_fighter(name: str, x: float):
  # The C++ Fighter snaps Y to the arena ground on creation, so only X is set
  obj = orx.object.create_from_config(name)
  if obj is not None:
    position = obj.get_position()
    position.x = x
    obj.set_position(position)
  return obj

def start_wave(n: int):
  global g_wave, g_timer

  g_wave  = n
  g_timer = 0.0

  set_state_int("Wave", n)
  set_state_float("WaveScale", 1.0 + 0.15 * (n - 1))
  set_state_string("Message", "WAVE %d" % n)
  set_state_float("MessageTimer", 1.5)

  section = "Wave%d" % n
  with orx.config.Section(section):
    count  = orx.config.get_list_count("EnemyList")
    base_x = orx.config.get_float("SpawnX")

  for i in range(0, count):
    with orx.config.Section(section):
      name = orx.config.get_string("EnemyList", i)
    x = base_x + 200.0 * i
    if x > 880.0:
      x = 880.0
    elif x < -880.0:
      x = -880.0
    g_enemies.append(spawn_fighter(name, x))

def start_match():
  global g_player, g_enemies, g_wave, g_timer

  g_enemies = []
  g_wave    = 0
  g_timer   = 0.0

  set_state_int("EnemiesAlive", 0)
  set_state_string("GameState", "Playing")

  g_player = spawn_fighter("Saya", 0.0)
  start_wave(1)

def request_restart():
  global g_player, g_enemies, g_timer

  # Never touch stale handles: schedule deletion, the C++ side keeps
  # [MatchState] bookkeeping symmetric through its OnDelete callbacks
  set_state_string("GameState", "Restarting")

  if g_player is not None:
    g_player.set_life_time(0.0)
  for enemy in g_enemies:
    if enemy is not None:
      enemy.set_life_time(0.0)

  g_player  = None
  g_enemies = []
  g_timer   = 0.0

# Engine callbacks -------------------------------------------------------------

def init():
  # Push [Main] as the default config section
  orx.config.push_section("Main")

  # Create viewports defined in [Main]
  for i in range(0, orx.config.get_list_count("ViewportList")):
    name = orx.config.get_string("ViewportList", i)
    orx.viewport.create_from_config(name)

  # Create the arena scene
  _ = orx.object.create_from_config("Game")

  # First match
  start_match()

def update(dt: float):
  global g_timer

  # Check to see if we should quit the game
  if orx.input.has_been_activated("Quit"):
    orx.close()
    return

  state = get_state_string("GameState")

  # Match over: wait for restart
  if (state == "GameOver") or (state == "Victory"):
    if orx.input.has_been_activated("Restart"):
      request_restart()
    return

  # Waiting for old combatants to be deleted
  if state != "Playing":
    return

  # Final wave cleared: victory
  if (g_wave >= MAX_WAVE) and (get_state_int("EnemiesAlive") <= 0):
    set_state_string("GameState", "Victory")
    return

  # All foes down: brief breather, then next wave
  if get_state_int("EnemiesAlive") <= 0:
    if g_timer == 0.0:
      set_state_string("Message", "WAVE CLEAR!")
      set_state_float("MessageTimer", CLEAR_DELAY)
    g_timer = g_timer + dt
    if g_timer >= CLEAR_DELAY:
      start_wave(g_wave + 1)
  else:
    g_timer = 0.0

def camera_update(dt: float):
  # Get camera proxy control object
  with orx.config.Section("MainCamera"):
    id = orx.config.get_guid("ID")
  camera = orx.object.from_guid(id)
  if camera is not None:
    # Round camera position
    position = camera.get_position()
    position.round()
    camera.set_position(position)

# Register camera update callback
orx.on_camera_update = camera_update

# Setup engine callbacks for pyscroll
pyscroll.setup(init, update)
