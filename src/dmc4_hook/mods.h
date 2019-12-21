#pragma once
#include "hacklib/PatternScanner.h"
#include "hacklib/Patch.h"
#include "hacklib/Hooker.h"
#include "imgui/imgui.h"
#include "config.h"

/*
bool memAssert(const char* bytes, uintptr_t location) {
	int result = std::memcmp(bytes, (const void*)location, sizeof(bytes));
	return result == 0;
}
*/
namespace modShared {
	void limitadjust_patch(void);
}
namespace modMoveIDs {
	bool init(uintptr_t base);
	void toggle(bool value);
};

namespace modSelCancels {
	bool init(uintptr_t base);
	void onGUIframe();
	void loadConfig(CONFIG& config);
	void saveConfig(CONFIG& config);
	//void toggle(bool value);
};

namespace modLimitAdjust {
	enum LIMIT_TYPE {
		SWORD_GUNS,
		JC,
		STYLE_SWITCH,
		TARGET_CHANGE
	};
	bool init();
	void onGUIframe();
	void loadConfig(CONFIG& config);
	void saveConfig(CONFIG& config);
	//void toggle(bool value, LIMIT_TYPE limit);
}
/* mod sample exposing functions/variables we might want to call from outside
namespace modSample {
	void loadConfig(CONFIG& config);
	void saveConfig(CONFIG& config);
	bool init();
	void onGUIframe();
	void toggle(bool value);
};
*/