# Photo Worlds — DSi hardware prototype

First native milestone for the photo-to-game project. This build is deliberately plain.

- Home: Capture game / View saved games (D-pad and A, or touch).
- Top screen: live DSi camera; A/L/R captures a 256×192 preview. Y switches camera; B returns.
- Review: A saves the photo and starts a short platformer. B retakes.
- Game: left/right move, A jumps, hold B to duck; START pauses. Collect three stars and reach the flag before 90 seconds. Falls return to a checkpoint. Winning reveals a small flower garden.
- Saved games: calendar with dated entries; A replays the selected world. Photos and checksums are stored in `sd:/photo-worlds/` using numbered `.pwg` files. Restarting the app preserves these files.

## Honest scope

This is an offline hardware test, not the finished semantic photo-to-game generator. It samples colours from the photo for the level palette; the obstacle layout is fixed. No object recognition, network upload, generated artwork, or API calls are implemented. Photos stay on the SD card. Camera and touchscreen behavior require real DSi testing. Game logic has host tests, but a successful compile alone does not establish hardware compatibility.

## Build

Use BlocksDS (`make`) or the repository's **Build DSi app** GitHub Actions workflow. Download the `photo-worlds-dsi` artifact from a successful run and unzip it.

## Install the test

Copy `photo-worlds.nds` to the SD card alongside `BOOT.NDS`. Do NOT replace `BOOT.NDS` or change the existing startup settings yet. Open Photo Worlds from TWiLight Menu++. If it reports DS mode, launch `photo-worlds.nds` directly from Unlaunch (hold A+B at power-on). Test the camera, capture, save, complete a level, then power off/on and replay from Saved games. Only after this passes should the app become the automatic startup target.

On a save failure the photo remains in memory on the review screen for retry. Up to 999 worlds are supported. Corrupt or truncated saves are rejected. Temporary incomplete saves are excluded from the gallery.

## Local tests

```
cc -Wall -Wextra -Werror source/game.c tests/game_test.c -o /tmp/photo-worlds-tests
/tmp/photo-worlds-tests
```

## Next milestones

1. Confirm camera capture and save/replay on Tarshaa's DSi.
2. Test DSi Wi-Fi and a hosted photo-processing service; no API secrets on SD.
3. Replace palette-only mapping with validated semantic level data. Preserve meaningful photo relationships, clear controls, temporal pressure and a small contextual reward.
4. Apply the visual design, extend the genre templates and configure direct startup after hardware verification.

## Calendar and Brooklyn test world
Saved games uses a monthly calendar. D-pad moves by day/week, L/R by month; A opens the selected game, X cycles games on the same date, B returns home. Touch a date to preview; touch it again to play. Dates with games have a dot. The original photo stays on the upper screen while the lower screen plays. New photos use the console date; old saves use file modification date. Set the DSi clock for meaningful dates.

A private `sd:/photo-worlds/brooklyn.pwg` sidecar adds Brooklyn — Special Delivery, dated 2026-10-18. This hand-authored photo-specific world has a bag delivery, striped awning, AC updraft and seated recipient. Hold Up to travel forward, Left/Right to steer across the street, Down to go back. Press A to jump, hold A to glide, hold B while moving to duck, START to pause. Existing captured worlds keep the palette-based prototype. Automatic photo-to-level interpretation is not implemented.

PWG files contain three little-endian uint32 values (magic, 49152 pixels, checksum), followed by 49152 RGB555 uint16 pixels and an optional YYYYMMDD uint32. Magic is 0x50574731 for camera worlds, 0x50574231 for the fixed Brooklyn world. Checksum is FNV-1a-style over uint16 values, not individual bytes. Old files without a date remain readable. The private source photo and generated sidecar are excluded from this public repository.

## Original game port
Brooklyn now ports the recovered October 13 Special Delivery browser prototype, replacing the side-scrolling substitute. It uses the original angled projection (x*22 + depth*5, depth*-13, height*-21 at reference 512x384), camera smoothing, street dimensions, gaps at 13–16/36–43/49–52, checkpoints at 20/32/48, low canopy at 26–29, AC lift zone, changing calm/tailwind/crosswind, recipient at depth 65 and bouquet ending. Reference drawing coordinates are reduced to the DSi's 256x192 display.

The original browser game uses an isometric/pseudo-3D projection with sprite characters, not a polygon engine. This port preserves that same presentation and movement. Native resolution, RGB555 color and rasterization differ; the browser overlay is adapted to the DSi buttons. Camera and photo-save behavior remain unchanged. No claim of pixel-identical browser output or real-hardware validation before testing.
