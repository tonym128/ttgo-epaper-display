import os
from SCons.Script import Import

Import("env")

# Locate .env in project dir or parent workspace dir
project_dir = env.get("PROJECT_DIR", "")
workspace_dir = os.path.abspath(os.path.join(project_dir, ".."))

env_file = None
for path in [
    os.path.join(workspace_dir, ".env"),
    os.path.join(project_dir, ".env"),
    os.path.join(workspace_dir, ".env.example"),
    os.path.join(project_dir, ".env.example"),
]:
    if os.path.isfile(path):
        env_file = path
        break

if env_file:
    print(f"--> [load_env.py] Loading configuration from: {env_file}")
    with open(env_file, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, val = line.split("=", 1)
            key = key.strip()
            val = val.strip()
            # Strip surrounding quotes if present
            if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
                val = val[1:-1]
            
            # Escape double quotes for C-preprocessor string literal
            escaped_val = f'\\"{val}\\"'
            env.Append(CPPDEFINES=[(key, escaped_val)])
else:
    print("--> [load_env.py] No .env or .env.example found; using config.h defaults.")
