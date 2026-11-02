# Plinko — Rebble Appstore

Copy these fields into [dev-portal.rebble.io](https://dev-portal.rebble.io/submit).

| Field | Value |
| --- | --- |
| App name | Plinko |
| Type | Watch app |
| Category | Games |
| Source code | https://github.com/TechJeeper/Plinko |
| `.pbw` | `build/Plinko.pbw` |
| CloudPebble zip | `dist/Plinko-cloudpebble.zip` |

## Short description

```
Aim, drop, bounce — classic Plinko on your wrist.
```

## Description

Paste into each platform asset collection (max 1600 characters):

```
Plinko is classic arcade drop-and-bounce for your wrist.

Aim the drop column with UP and DOWN, then press SELECT to release the ball. It ricochets through staggered pegs into scoring bins — center pays more (up to 10x).

• Aim and drop with the side buttons
• Peg physics with scoring bins (1-3-5-10-5-3-1)
• Rounds of 5, 10, or 15 balls
• Persistent high score
• Easy / Normal / Hard fall speed & bounce chaos
• Optional vibration and aim guide
• Custom background, peg, and ball colors (color watches)

Configure difficulty, balls per round, haptics, and colors from the Pebble phone app settings page (Clay).

Works on Pebble Time 2, Pebble Time, Pebble 2, and Pebble 2 Duo.
```

## Release notes (v1.0.0)

```
Initial release — aim, drop, chase the 10x bin.
```

## Screenshots

Exact display sizes (no bezel). Regenerate with:

```powershell
python tools/generate_store_screenshots.py
```

| Platform | Size | Folder |
| --- | --- | --- |
| basalt (Pebble Time) | 144×168 | `store/screenshots/basalt/` |
| diorite (Pebble 2) | 144×168 | `store/screenshots/diorite/` |
| flint (Pebble 2 Duo) | 144×168 | `store/screenshots/flint/` |
| emery (Pebble Time 2) | 200×228 | `store/screenshots/emery/` |

Scenes: title, aim, falling, scored, round end.
