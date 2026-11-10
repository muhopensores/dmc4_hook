#pragma once

#include <optional>
#include <string>

// CoopHud: the HUD fitted to each player's part of the screen. A HUD piece (uDevilCock: uCockpitNero / uCockpitDante,
// uMinimap, uLockOnCock) draws in its view's pixels from the view's top-left at full-screen size, so in a split screen
// each HUD took half its viewport. Every one draws through uDevilCock::draw 0x532F50 (vtbl +0x24; the game's
// uCockpitMgr 0x507610 calls it for its own list), and every sprite and glyph ends in sPrim::drawSprite 0x45B410 with
// its four corners in eax (x / y at +0 / +4, 0x10 apart; built by sub_45A420 for sprites, sub_45ABA0 for text). While a
// piece other than the reticle draws, the corners are scaled about the view's top-left. The reticle (uLockOnCock /
// uLockOnDante) is placed by projecting its target, so it keeps its size and place.
// The reticle's draw check (sub_76FAA0, both classes) picks the target from mpPlayer and shows it only while the
// target's mIsCameraIn is set, and runs in the draw pass, outside every unit's update: player 2's reticle drew player
// 1's target. The draw pass runs beside the next frame's updates on another thread, so it can't swap the context
// (that raced the update swaps and left player 2 as the game's player). Instead the check runs at the end of each
// player 2-4 reticle's update, in his context (after_update), and the draw takes that answer.
// Its place: sub_A592F0 projects through the draw context's matrix and viewport, which in a split screen's HUD pass
// aren't the reticle's view, so while split a reticle is projected through its view's own matrices (sCamera viewport
// mViewMat / mProjMat / mRegion).

namespace coop_hud {

extern float hud_scale;   // the HUDs' size while players 2-4 are in (0.5 fits a half or a quarter of the screen)
extern bool split_shown;  // the views are split now (Coop's update_split; the HUD scale applies only then)

std::optional<std::string> install();   // the hooks; an error text on failure
// Called after a unit's update, still in its player's context.
void after_update(uintptr_t unit);

} // namespace coop_hud
