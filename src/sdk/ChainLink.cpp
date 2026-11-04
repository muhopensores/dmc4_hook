#include "ChainLink.hpp"
#include "CnsChain.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

namespace chain_link {

namespace {
// cCnsChain, in the unit (RE note chain_cnschain.md section 2): mBone[32] s16 (-1 between tendrils), the solved points
// mat[33] (0x40 each, the translation row at +0x30; the tip's tail sits in the -1 slot after it), vel[33] (0x10 each)
constexpr uintptr_t kBones = cns_chain::kChainInUnit + 0x20;
constexpr uintptr_t kMats = cns_chain::kChainInUnit + 0x560;
constexpr uintptr_t kVels = cns_chain::kChainInUnit + 0xDA0;
constexpr int kSlots = 32, kPasses = 4;
// uModel (model_attachments.md): joints 0x90 each, +0x05 parent index (0xFF = the model), +0x50 mWmat
constexpr uintptr_t kJointNum = 0x2E0, kpJoint = 0x2E4, kJointTable = 0x2E8, kJointSize = 0x90, kJParent = 0x05, kJWmat = 0x50;

template <class T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

int bone(uintptr_t chain, int slot) { return slot >= 0 && slot < kSlots ? at<int16_t>(chain, kBones + slot * 2) : -1; }
float* mat(uintptr_t chain, int node) { return &at<float>(chain, kMats + node * 0x40); }
float* vel(uintptr_t chain, int node) { return &at<float>(chain, kVels + node * 0x10); }

// An MT matrix's rows (x, y, z, position) are a glm matrix's columns: glm's M * v is the game's v . M.
glm::mat4 load(const float* m) { return glm::make_mat4(m); }
void store(float* m, const glm::mat4& g) { std::memcpy(m, glm::value_ptr(g), sizeof(float) * 16); }

// The smallest turn taking direction `from` onto `to`.
glm::quat turn(const glm::vec3& from, const glm::vec3& to) {
    const float lf = glm::length(from), lt = glm::length(to);
    if (lf < 1e-5f || lt < 1e-5f) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    const glm::vec3 a = from / lf, b = to / lt, c = glm::cross(a, b);
    const float s = glm::length(c);
    if (s < 1e-6f) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::angleAxis(std::atan2(s, glm::dot(a, b)), c / s);
}

// A tendril point being worked on: a joint slot, or the tip's tail (no joint). A tendril's nodes are stored root to tail.
struct Node {
    uintptr_t chain;
    int slot;
    int joint;                          // index into the model's joints, -1 for the tail
    int first, last;                    // the tendril's nodes in the work list
    glm::vec3 old_pos, pos;
};

struct Work {
    uintptr_t model = 0, joints = 0;
    uint32_t joint_num = 0;
    const uint8_t* table = nullptr;
    std::vector<Node> nodes;

    float* wmat(int index) { return &at<float>(joints + index * kJointSize, kJWmat); }

    // The work-list index of (chain, slot), adding its whole tendril; -1 when a joint of it is missing on the model.
    int node_of(uintptr_t chain, int slot) {
        for (size_t i = 0; i < nodes.size(); ++i) if (nodes[i].chain == chain && nodes[i].slot == slot) return (int)i;
        int root = slot, tip = slot;
        while (root > 0 && bone(chain, root - 1) >= 0) --root;
        while (tip + 1 < kSlots && bone(chain, tip + 1) >= 0) ++tip;
        const int first = (int)nodes.size(), last = first + (tip - root) + 1;   // + the tail
        std::vector<Node> add;
        for (int s = root; s <= tip + 1; ++s) {
            Node n{ chain, s, -1, first, last, {}, {} };
            const float* m = mat(chain, s);
            if (s <= tip) {
                const int no = bone(chain, s);
                const uint8_t i = table[no & 0xFF];
                if (i == 0xFF || i >= joint_num) return -1;
                n.joint = i;
                m = wmat(i);
            }
            n.old_pos = n.pos = glm::vec3(m[12], m[13], m[14]);
            add.push_back(n);
        }
        nodes.insert(nodes.end(), add.begin(), add.end());
        return first + (slot - root);
    }

    bool root(int i) const { return i == nodes[i].first; }

    // Node i moved by c, kept at its distance from its parent (it turns about it); the rest of its tendril follows.
    void move(int i, const glm::vec3& c) {
        if (root(i)) return;
        const glm::vec3 p = nodes[i - 1].pos, target = nodes[i].pos + c;
        const float len = glm::length(nodes[i].pos - p), d = glm::length(target - p);
        const glm::vec3 to = (len < 1e-4f || d < 1e-4f) ? target : p + (target - p) * (len / d);
        const glm::vec3 delta = to - nodes[i].pos;
        for (int j = i; j <= nodes[i].last; ++j) nodes[j].pos += delta;
    }
};
} // namespace

int slot_of(uintptr_t chain, int joint_no) {
    if (!chain || joint_no < 0) return -1;
    for (int s = 0; s < kSlots; ++s) if (bone(chain, s) == joint_no) return s;
    return -1;
}

bool is_root(uintptr_t chain, int slot) { return slot == 0 || bone(chain, slot - 1) < 0; }

void solve(uintptr_t model, const Link* links, size_t count) {
    if (!model || !links || !count) return;
    Work w;
    w.model = model;
    w.joints = at<uintptr_t>(model, kpJoint);
    w.table = at<const uint8_t*>(model, kJointTable);
    w.joint_num = at<uint32_t>(model, kJointNum);
    if (!w.joints || !w.table || !w.joint_num) return;   // setModel is swapping the joints
    struct Pair { int a, b; float max, stiffness; bool stretched; };
    std::vector<Pair> pairs;
    for (size_t k = 0; k < count; ++k) {
        const Link& l = links[k];
        const int a = w.node_of(l.chain_a, l.slot_a), b = a < 0 ? -1 : w.node_of(l.chain_b, l.slot_b);
        if (a < 0 || b < 0 || a == b || (w.root(a) && w.root(b))) continue;
        pairs.push_back({ a, b, l.max, l.stiffness, glm::length(w.nodes[b].pos - w.nodes[a].pos) > l.max });
    }
    bool moved = false;
    for (int pass = 0; pass < kPasses; ++pass)
        for (const Pair& p : pairs) {
            const glm::vec3 d = w.nodes[p.b].pos - w.nodes[p.a].pos;
            const float len = glm::length(d);
            if (len <= p.max || len < 1e-4f) continue;
            const float wa = w.root(p.a) ? 0.0f : 1.0f, wb = w.root(p.b) ? 0.0f : 1.0f;
            const glm::vec3 c = d * ((len - p.max) * p.stiffness / (len * (wa + wb)));
            w.move(p.a, c * wa);
            w.move(p.b, -c * wb);
            moved = true;
        }
    if (!moved) return;
    // the speed apart: what made the link stretch, and what the next integration would add again
    for (const Pair& p : pairs) {
        if (!p.stretched) continue;
        const glm::vec3 d = w.nodes[p.b].pos - w.nodes[p.a].pos;
        const float len = glm::length(d);
        if (len < 1e-4f) continue;
        const glm::vec3 n = d / len;                    // a -> b
        for (const auto& [i, toward] : { std::pair<int, glm::vec3>{ p.a, n }, std::pair<int, glm::vec3>{ p.b, -n } }) {
            if (w.root(i)) continue;
            float* v = vel(w.nodes[i].chain, w.nodes[i].slot);
            const float away = v[0] * toward.x + v[1] * toward.y + v[2] * toward.z;
            if (away < 0.0f) { v[0] -= toward.x * away; v[1] -= toward.y * away; v[2] -= toward.z * away; }
        }
    }
    // each moved joint: turned to aim at its next point again, placed on its own; joints below that no tendril of the work
    // holds take their parent's change (joint array order is parents first)
    std::vector<glm::mat4> delta(w.joint_num, glm::mat4(1.0f));
    std::vector<uint8_t> state(w.joint_num, 0);        // 1 a tendril joint of the work, 2 follows its parent
    for (size_t i = 0; i < w.nodes.size(); ++i) {
        const Node& n = w.nodes[i];
        if (n.joint < 0) continue;
        const Node& next = w.nodes[i + 1];              // a joint node always has one: the next joint or the tail
        state[n.joint] = 1;
        const glm::quat q = turn(next.old_pos - n.old_pos, next.pos - n.pos);
        delta[n.joint] = glm::translate(glm::mat4(1.0f), n.pos) * glm::mat4_cast(q) * glm::translate(glm::mat4(1.0f), -n.old_pos);
    }
    for (uint32_t j = 0; j < w.joint_num; ++j) {
        if (!state[j]) {
            const uint8_t parent = at<uint8_t>(w.joints + j * kJointSize, kJParent);
            if (parent == 0xFF || parent >= w.joint_num || !state[parent]) continue;
            delta[j] = delta[parent];
            state[j] = 2;
        }
        store(w.wmat(j), delta[j] * load(w.wmat(j)));
    }
    // the chains' own points: the next frame's solve starts from them
    for (const Node& n : w.nodes) {
        float* m = mat(n.chain, n.slot);
        if (n.joint >= 0) std::memcpy(m, w.wmat(n.joint), sizeof(float) * 16);
        else { m[12] = n.pos.x; m[13] = n.pos.y; m[14] = n.pos.z; }
    }
}

} // namespace chain_link
