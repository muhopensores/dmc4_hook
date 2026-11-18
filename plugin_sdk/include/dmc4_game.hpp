// dmc4_game.hpp: the game's types for plugins (MSVC, 32-bit, C++20): MT Framework objects, units, players, systems.
//
// These are dmc4_hook's own SDK headers (src\sdk), the same files the hook compiles against, limited to the ones that
// only describe the game's memory and call its fixed code. The game's executable never changes, so these layouts don't
// either: a plugin built against an older copy keeps working with a newer dmc4_hook (it knows fewer field names). The
// hook's own services (src\sdk\*.cpp: Devil4, MotionRegistry, ChainSolver, Players, CustomUnit ...) aren't here: plugins
// reach them through dmc4_plugin.h.
//
// Include paths for a plugin: plugin_sdk\include, src, src\sdk and dependencies\glm of the dmc4_hook repo.
//
// Rules
// - Cast the void* that api->game returns to these types (DMC4_OBJ_MEDIATOR -> sMediator*, a line-13 unit -> uPlayer*).
// - Never construct these classes (new, on the stack) and don't define their virtual functions: they mirror the game's
//   vtables. Calling one through a game object's pointer is fine (it dispatches through the game's own vtable).
// - The game changes these objects on its own thread: read and write them from on_frame or a hook on the game's code.
// - ReClass_Internal.hpp holds the older ReClass dumps (uPlayer_old ...); prefer the typed headers (uPlayer.hpp ...).
#pragma once

#ifndef DMC4_PLUGIN_SDK
#define DMC4_PLUGIN_SDK   // tells the shared headers they're in a plugin (no hook-only parts)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// math and MT Framework basics
#include "sdk/Math.hpp"
#include "sdk/MtMath.hpp"
#include "sdk/MtObject.hpp"
#include "sdk/MtDTI.hpp"
#include "sdk/MtAllocator.hpp"
#include "sdk/MtString.hpp"
#include "sdk/MtThread.hpp"
#include "sdk/MtMemoryStream.hpp"
#include "sdk/Enums_Internal.hpp"

// systems (singletons: api->game->object)
#include "sdk/cSystem.hpp"
#include "sdk/sDevil4Main.hpp"
#include "sdk/sUnit.hpp"
#include "sdk/sArea.hpp"
#include "sdk/aGame.hpp"
#include "sdk/sMediator.hpp"
#include "sdk/sRender.hpp"
#include "sdk/sMouse.hpp"
#include "sdk/sStylishCount.hpp"
#include "sdk/sWorkRate.hpp"
#include "sdk/Pad.hpp"
#include "sdk/Save.hpp"

// resources
#include "sdk/cResource.hpp"
#include "sdk/rEffectList.hpp"

// units
#include "sdk/cUnit.hpp"
#include "sdk/uCoord.hpp"
#include "sdk/uActor.hpp"
#include "sdk/uCollisionMgr.hpp"
#include "sdk/uDamage.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/uEm003.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/uPlayerDante.hpp"
#include "sdk/uPlayerData.hpp"
#include "sdk/NeroStand.hpp"
#include "sdk/uLight.hpp"
#include "sdk/uShadow.hpp"
#include "sdk/uFilter.hpp"
#include "sdk/uStageSet.hpp"
#include "sdk/uMissionMenu.hpp"
#include "sdk/uMissionStart.hpp"
#include "sdk/Cam.hpp"

// menus: layouts (sprMap / fontMap / rSprLayout), uListSelect and its vtable, the title, pause and practice menus
#include "sdk/Menus.hpp"

// .ctn cancel scripts built in memory (for api->game->install_ctn_script)
#include "sdk/CtnScript.hpp"

// the older ReClass dumps (uPlayer_old, sMediator's old layout ...): kept for code that still uses them
#include "sdk/ReClass.hpp"
#include "sdk/ReClass_Internal.hpp"
