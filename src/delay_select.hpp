#pragma once

// ============================================================================
//  Dynamic 4-8 frame delay selection (pure logic, no Geode dependency)
//
//  UNITS
//    GD 2.2 runs its physics at 240 ticks per second. PlayerObject::update()
//    receives dt = 0.25 per tick (dt is normalized to 1/60 s), so:
//        1 tick  = 1/240 s = 0.25 frame @ 60 FPS
//        1 frame @ 60 FPS = 4 ticks
//    4-8 frames @ 60 FPS  = 16-32 ticks.
//    Every delay in this mod is stored and counted in TICKS.
// ============================================================================

#include <algorithm>
#include <vector>

namespace pib {

constexpr int TICKS_PER_FRAME_60 = 4;

constexpr int MIN_DELAY_FRAMES = 4;
constexpr int MAX_DELAY_FRAMES = 8;
constexpr int MIN_DELAY_TICKS  = MIN_DELAY_FRAMES * TICKS_PER_FRAME_60;  // 16
constexpr int MAX_DELAY_TICKS  = MAX_DELAY_FRAMES * TICKS_PER_FRAME_60;  // 32

// Step used while scanning the 16..32 tick window. 1 = every tick (most
// precise). 2 or 4 = fewer simulations per click (4 = whole frames only).
constexpr int DELAY_SCAN_STEP_TICKS = 1;

// A delayed input only counts as "saving" the player if it outlives the
// immediate input by at least this many ticks beyond what plain geometry
// explains (see scoreCandidate).
constexpr int WAVE_GAIN_MARGIN_TICKS    = 8;   // 2 frames
constexpr int GENERIC_GAIN_MARGIN_TICKS = 12;  // 3 frames

struct DelayCandidate {
    int  delay     = 0;   // ticks of delay being evaluated
    int  window    = 0;   // post-input ticks that were simulated (max)
    int  survived  = 0;   // ticks survived AFTER the delayed input was applied
    int  alive     = 0;   // absolute ticks alive counted from the click
                          //   (death index, or delay + window if it never died)
    int  score     = 0;   // alive, minus the natural Wave corridor gain
    bool clear     = false;      // survived the whole simulated window
    bool qualified = false;      // clearly better than the immediate input
};

// Post-input window to simulate for a given delay.
// Wave: delaying a turn by D ticks makes the wave travel D ticks the wrong way
// first, so a plain corridor already lasts ~2*D ticks longer in absolute time.
// The window is stretched by D (+margin) so that this natural gain can never be
// hidden by the end of the window (otherwise long delays would look "clear"
// just because the simulation stopped).
inline int simWindow(bool isWave, int horizon, int delay) {
    return isWave ? horizon + delay + WAVE_GAIN_MARGIN_TICKS : horizon;
}

// Fills clear / score / qualified. frame0Alive = ticks the player lives when
// the input is NOT delayed (always < horizon here, otherwise no scan happens).
inline void scoreCandidate(DelayCandidate& c, int frame0Alive, bool isWave) {
    c.clear = c.survived >= c.window;
    c.score = isWave ? (c.alive - 2 * c.delay) : c.alive;
    int margin = isWave ? WAVE_GAIN_MARGIN_TICKS : GENERIC_GAIN_MARGIN_TICKS;
    // survived > 0: the player must still be alive when the delayed input lands.
    c.qualified = c.survived > 0 && (c.score - frame0Alive) >= margin;
}

// Returns the index of the chosen candidate, or -1 if no delay beats the
// immediate input (the caller then sends the input immediately).
//
// 1) If some delays clear the whole window, pick the MOST ROBUST one: the
//    delay in the middle of the longest run of consecutive safe delays (largest
//    distance to a delay that fails). Ties -> smaller delay (less latency).
//    That is why the result is neither always 4 nor always 8 frames.
// 2) Otherwise pick the qualified delay that lives longest (score), ties ->
//    smaller delay.
inline int selectBestCandidate(const std::vector<DelayCandidate>& c) {
    const int n = static_cast<int>(c.size());
    int best = -1;
    int bestMargin = -1;

    for (int i = 0; i < n; i++) {
        if (!(c[i].clear && c[i].qualified)) continue;
        int left = 0, right = 0;
        for (int j = i - 1; j >= 0 && c[j].clear && c[j].qualified; j--) left++;
        for (int j = i + 1; j < n && c[j].clear && c[j].qualified; j++) right++;
        int margin = std::min(left, right);
        if (margin > bestMargin) {
            bestMargin = margin;
            best = i;
        }
    }
    if (best >= 0) return best;

    int bestScore = -1;
    for (int i = 0; i < n; i++) {
        if (!c[i].qualified) continue;
        if (c[i].score > bestScore) {
            bestScore = c[i].score;
            best = i;
        }
    }
    return best;
}

} // namespace pib
