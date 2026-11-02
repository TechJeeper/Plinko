# Plinko

Classic Plinko arcade game for Pebble / Rebble watches (SDK 3).

Source: https://github.com/TechJeeper/Plinko

Aim the drop column with **UP** / **DOWN**, press **SELECT** to drop. Bounce through pegs into scoring bins. High score persists on the watch.

Configure difficulty, balls per round, haptics, aim guide, and colors from the phone app settings page (Clay).

## Platforms

- emery (Pebble Time 2)
- basalt (Pebble Time)
- diorite (Pebble 2)
- flint (Pebble 2 Duo)

## Build (Windows)

Requires Podman Desktop or Docker Desktop.

```powershell
./build.ps1
./build.ps1 -Phone 192.168.1.23
./build.ps1 -Zip
```

Output: `build/Plinko.pbw`

## Controls

| Button | Action |
| --- | --- |
| SELECT | Start / drop / play again |
| UP | Aim left (or return to title after a round) |
| DOWN | Aim right |

## Assets

Menu icon, ball, peg, and title splash were generated with [Pixellab](https://pixellab.ai) (1-bit palette) and converted to white-on-transparent PNGs for Pebble `GCompOpSet`.
