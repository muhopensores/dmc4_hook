#pragma once

#include "../mod.hpp"
#include "misc/kAtckDefTbl.cpp"

extern uintptr_t HookDanteKADTbl;
extern uintptr_t HookNeroKADTbl;
struct kAtckDefTbl;

class MoveTable: public Mod {
public:
    // static bool mod_enabled;
    static uintptr_t jmp_ret0;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;   // Nero dispatch (uPlayerNero::moveActAtckL 0x7F9CDB)

    static int extra_nero_moves;
    static int extra_dante_moves;

    static int AirThrow;
    static int backForwardQuickDrive;
    static int Gerb;             // Nero row index marker (extra_nero_moves at insertion), see Gerb.cpp
    static int GerbVertical;     // its second row (the up / down jump, id 0x41)
    static int DanteAirDodge;    // Dante row index marker (extra_dante_moves at insertion), see DanteAirDodge.cpp

    // A row from another mod (MoveDef): inserted at the front of `character`'s table (0 Nero, 1 Dante) like the ones
    // above, 14 words = kAtckDefTbl. Returns the marker; `row(character, marker)` finds it again after later insertions.
    // Rows must be added before the game hands the table to a player (on_initialize), or that player's pointer dangles.
    static int add_row(int character, const uint32_t words[14]);
    static kAtckDefTbl* row(int character, int marker);

    MoveTable() = default;

    // void toggle(bool enable);

    void display_attack_entry(kAtckDefTbl* TblEntry);

    std::string get_mod_name() override { return "MoveTable"; };
    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
private:
    std::unique_ptr<FunctionHook> hook0, hook1, hook2, hook3, hook4;
    // std::unique_ptr<Patch> patch1;

    static const int COLUMN_SEARCH_BUFFER_SIZE = 32;
    char columnSearchBuffers[18][COLUMN_SEARCH_BUFFER_SIZE] = {0};
    bool columnSearchActive[18] = {false};
    bool columnSearchIsExact[18] = {false};
    bool enableExactMatch = true;
    
    std::string formatHex(uint32_t value) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%08x", value);
        return std::string(buffer);
    }
};
