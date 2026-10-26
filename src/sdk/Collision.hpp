#pragma once

#include <cstdint>
#include "MtMath.hpp"

// Line casts against the stage collision (the .sbc meshes): sDevil4Collision::findIntersection, the query the game's
// own wall / ground / camera checks use (RE notes collision_lineCast.md, sCollision_LineSeg_findings.md). Addresses are
// DX9. Shared by WallRun (the wall checks) and ThirdPersonAim (the crosshair ray, camera collision); RayTest keeps its
// own copy.
namespace collision {

// sCollision filters: the channels a cast collides in (RayTest's names).
enum COLLISION_FILTER : uint32_t {
    COLL_DEFAULT     = 0x10100,
    COLL_WALL        = 0x20000,     // what checkWallJumpBegin casts in
    COLL_PLAYER      = 0x20200,
    COLL_SHELL       = 0x40400,     // the bullets
    COLL_GROUND      = 0x80800,
    COLL_ENEMY_SHELL = 0x101000,
    COLL_HAND        = 0x202000,
    COLL_CAMERA      = 0x404000,    // the walls the game's camera stops at
    COLL_FALLSTOP    = 0x10000000,  // invisible walls (what stops Stinger at a ledge)
    COLL_CAMERASTOP  = 0x20000000,  // camera-only walls
    COLL_ALL         = 0xFFFFFFFF,
};

// sCollision::Param, as RayTest fills it (sCollision::Param(127, 0x20000, 0, 0, 10) in checkWallJumpBegin).
#pragma pack(push, 2)
struct CollParam {
    int type;                 // 127
    int filter;               // COLLISION_FILTER bits
    int hitCallback_this, hitCallback;
    void* hitCallback_data;
    int _pad14;
    int handle_exclude_0, handle_exclude_1;   // -1
    int _pad20;
    int correctionNum;        // 10
    unsigned __int8 bAdjMvRelSet, _pad29, bAdjAntiStop, _pad2B;
    float capsuleFwdLimit, capsuleBwdLimit;   // FLT_MAX
    float sweepRadiusCap;     // 0
    int partsFilter;          // 0
    int _pad3C;
    char pad[0x10];
};
#pragma pack(pop)

// sCollision::LineResult, the hitOut of findIntersection (RayTest's LineInfo; uPlayer::mGroundPoly is one).
struct LineResult {
    void* pSbc;               // 0x00
    void* tag;                // 0x04
    void* pPartsInfo;         // 0x08
    uint8_t* pTriangle;       // 0x0C  28-byte .sbc triangle: +0x14 surface_attr (bit 0 = no wall jump)
    uint8_t* pVertex;         // 0x10  16-byte vertices
    char pad14[0xC];
    MtVector3 triPlaneNormal; // 0x20
    MtVector3 triEdge1;       // 0x30
    MtVector3 triEdge2;       // 0x40
    MtVector3 hitPlane;       // 0x50  (RayTest: hitNormal)
    MtVector3 hitPos;         // 0x60
    MtVector3 hitNormal;      // 0x70  (RayTest: sweepDirNeg; which one it is isn't settled, use tri_normal)
    float hitDistance;        // 0x80
    char pad84[0xC];
};
static_assert(sizeof(LineResult) == 0x90, "LineResult is the 0x90 PlayerLineResult");

// A line query in `filter`, filled as the game's own casts are (type 127, no excluded handles, 10 corrections).
CollParam line_param(uint32_t filter);

// sDevil4Collision::findIntersection 0x95B320 (query@eax, the instance, segment, both sides, hit out): true on a hit,
// `out` holding the nearest one. One-sided unless both_sides (a triangle is hit only from the side its normal faces).
bool find_intersection(CollParam* param, MtLineSegment* segment, bool both_sides, LineResult* out);

// sMath::getNormalToTri 0x48F1A0: the hit triangle's unit normal.
MtVector3 tri_normal(const LineResult* hit);

} // namespace collision
