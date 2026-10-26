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

// sDevil4Collision line, any hit (sub_94B660, 29 game callers): true if the segment touches anything in `filter`.
bool line_hits(const MtLineSegment& segment, uint32_t filter, uint32_t type_mask = 0x7F);

// ---------------------------------------------------------------------------------------------------------------
// Shapes (RE note collision_shape_queries.md; the conventions are read from each function's prologue / epilogue and a
// game call site, not yet run from here). DX9 has no first-hit shape sweep: its "sweeps" are MT's move-and-slide, a
// shape carried from `old` to `pos` that slides along what it meets and gives back the corrected position. A slide
// query takes the shape RELATIVE to the moving point (the camera's is a sphere (0, 0, 0, r)); the overlap queries
// take it in world space. Game space throughout: cm, Y up. Shapes are MtMath's: MtSphere {x, y, z, r},
// MtCapsule {p0 @0, p1 @0x10, r @0x20}, MtAABB {min, max}.
//
// type_mask: the .sbc kinds to test (the 3rd argument of registResource): 1 the room, 2 boss / special, 0x10 the stage
// set gimmicks (MoveFloor ...), 0x40 the catapult; 0x7F all. The slide queries take it from the CollParam (`type`;
// 0 = nothing collides there). correctionNum in a CollParam = slide iterations (10 normal, 1 stops at the first contact).
//
// Slide results: an OR of 1 floor, 4 wall, 8 ceiling (0 = no contact). sphere_contact's class: 1 n.y above the slope
// limit, 2 above the wall limit, 4 wall, 8 ceiling.

// The sphere's nearest contact (sub_94B480): the triangle closest to `world` among those within its radius. On a
// hit `out_pos` = the closest point (world) and `out_plane` = its plane (n, d); both optional. Returns the contact
// class (0 = none). type_mask 0 = the default set. The game: filter 0x10100, type_mask 0x11, r 100.
int sphere_contact(const MtSphere& world, uint32_t filter, MtVector3* out_pos = nullptr, float* out_plane = nullptr, uint32_t type_mask = 0);

// Does the sphere overlap anything (sub_9564E0)? The CollParam's type, filter and groups apply.
bool sphere_overlap(CollParam* param, const MtSphere& world);

// Move a sphere from `old` to `pos` and slide (sub_956750): `pos` comes back corrected (always rewritten; old == pos
// = a pure push-out). `offset` is the sphere relative to the moving point. Returns the slide result.
int sphere_slide(CollParam* param, MtVector3* pos, const MtVector3& old, const MtSphere& offset);

// Move a capsule from `start` to `end` and slide: the actors' push capsule against the stage (cUtil::checkScrCollision
// 0x45F650, which builds its own query: type 127, all groups, 10 iterations). `offset` relative to the moving point;
// `out` = the corrected end, written only when the result is non-zero. The game (uEm012::adjustDodgePos): filter
// 0x307F7F00, start = the position + 50 up.
uint32_t capsule_slide(const MtCapsule& offset, const MtVector3& start, const MtVector3& end, MtVector3* out, uint32_t filter);
// The same with a query of your own (sub_956C90, what uCollisionMgr's movement uses): `pos` in / out as sphere_slide.
int capsule_slide(CollParam* param, MtVector3* pos, const MtVector3& old, const MtCapsule& offset);

// Does the capsule overlap anything (sub_94F230, collNarrow_Capsule)? The game: filter 0x80000, all kinds and groups.
bool capsule_overlap(const MtCapsule& world, uint32_t filter, uint32_t type_mask = 0x7F);

// Push a capsule out of what it penetrates (0x94EF30, the IDB's misnamed multi_obb_query): up to 64 triangles, the
// deepest resolved first; `cap` (world) comes back moved. `ref` is the reference capsule (the camera passes the same
// one). True = moved. The camera: {look-at, eye, r 10}, filter COLL_CAMERASTOP, both sides.
bool capsule_push_out(MtCapsule* cap, const MtCapsule& ref, uint32_t filter, bool both_sides = true, uint32_t type_mask = 0x7F);

// The triangles in a box. The BVH walk (query_all_sbcs, thunk 0x94B7C0) calls `cb` for every triangle of the leaves
// overlapping `box` (padded 1 cm) that passes the filter; the triangle itself is NOT tested against the box, do that in
// the callback (get_triangle). Moving parts are walked in their local space: found only at their rest position.
// Return non-zero from `cb` to report a contact (the results are ORed into the return value).
struct SbcInfo {                    // 0xB0, what the walk hands each triangle
    void*    pSbc;                  // 0x00
    int      sbcIndex;              // 0x04
    void*    node;                  // 0x08
    void*    part;                  // 0x0C
    int      triIndex;              // 0x10  within its group
    void*    group;                 // 0x14  the group record
    uint8_t* pTriangle;             // 0x18  28-byte .sbc triangle
    uint8_t* pVertex;               // 0x1C  16-byte vertices
    uint8_t  enable;                // 0x20
    char     pad21[0xF];
    float    mCoord[16];            // 0x30  the part's matrix (rows)
    float    mCoordOld[16];         // 0x70
};
static_assert(sizeof(SbcInfo) == 0xB0, "SbcInfo");
typedef int (__cdecl* TriCallback)(SbcInfo* tri, void* ctx);
int triangles_in_aabb(const MtAABB& box, TriCallback cb, void* ctx, uint32_t filter, uint32_t type_mask = 0x7F);
// The triangle's three vertices (getTriangle 0x948660), x y z w each, in the part's local space: transform by mCoord.
void get_triangle(const SbcInfo* tri, float out[12]);

// No OBB query: the OBB narrow phases only run inside the slide core. Use a capsule / sphere, or triangles_in_aabb
// with an OBB-vs-triangle test.

} // namespace collision
