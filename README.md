Doesn't matter if Click Between Frames is on.

## Dynamic 4-8 frame delay

When an immediate input would hit a spike, sawblade, slope or wall, the mod simulates every delay from
4 to 8 frames @ 60 FPS (16-32 physics ticks @ 240 TPS) and automatically picks the one with the best
predicted outcome. If no delay beats the immediate input, the input is sent immediately.

F6 (in level) toggles: 4-8 frames (auto) -> OFF.
