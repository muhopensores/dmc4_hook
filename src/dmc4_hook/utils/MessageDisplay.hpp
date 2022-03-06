#include "../imgui_dmc4.h"
#include <deque>
#include <string>
#include "../mod.hpp"
#define SHOW_MESSAGES()  MessageDisplay::instance().show_messages();
#define UPDATE_MESSAGE()  MessageDisplay::instance().update_messages();
#define DISPLAY_MESSAGE(format) MessageDisplay::instance().add_message(format);


struct message {

	message(std::string msg) : m_message(std::move(msg)) {
	}

	std::string m_message;
	float m_time = 100.0f;


	void show();
	// would be too fast for people playing on uncapped framerate
	// change m_time or fuck with deltaTime
	bool update(float dt);
};

struct MessageDisplay {
	std::deque<message> m_messages;

	// WOOO singletons, bad practice my ass
	static MessageDisplay& instance()
	{
		static MessageDisplay *instance = new MessageDisplay();
		return *instance;
	}

	void add_message(std::string msg);
	void show_messages();
	void update_messages();

private:
	MessageDisplay() {};
};

// Added this for UI checkbox pls dont hate me
// Define ModName class that is a Mod
class MessageDisplayMod : public Mod
{
public:
    // using default constructor
    MessageDisplayMod() = default;

    // override getModName() method with your mod name for logging.
    std::string getModName() override { return "MessageDisplayMod"; };

    // override onInitialize() method with routines required to initialize the mod.
    // return Mod::onInitialize() on success or error string on failure.
    std::optional<std::string> onInitialize() override;

    //void onFrame(fmilliseconds& dt) override;
    void onGUIframe() override;
    //void onTwitchCommand(std::size_t hash) override;
    //void onUpdateInput(hl::Input& input) override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
};
