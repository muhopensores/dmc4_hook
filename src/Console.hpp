#pragma once

#include "csys/system.h"
#include "imgui.h"
#include <array>

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include "d3d9.h"

struct ImGuiSettingsHandler;
class ImGuiConsole
{
public:

	/*!
	 * \brief Construct an imgui console
	 * \param c_name Name of the console
	 * \param inputBufferSize Maximum input buffer size
	 */
	explicit ImGuiConsole(std::string c_name = "imgui-console", size_t inputBufferSize = 256);

	/*!
	 * \brief Render the Dear ImGui Console
	 */
	void Draw(bool& open);

	/*!
	 * \brief Console system which handles the console functionality (Logging, Commands, History, Scripts, etc).
	 * \return System Obj
	 */
	csys::System& System();

	// could be done in constructor but we need to defer till imgui init :(
	void InitImgui();

	float displayFrac;	             // approaches finalFrac at con_speed
	float finalFrac;	             // 0.0 to 1.0 lines of console to display
	int	  fracTime;		             // time of last displayFrac update

	void SetDisplayFraction(float frac);
	void UpdateDisplayFraction();

	bool dump_file(const std::string& filename);

#if 0
	template <typename... Param> static void print(std::string_view format, Param && ... param) {
		console->m_ConsoleSystem.Log() << fmt::format(format, std::forward<Param>(param)...);
	}
#else
	void print(const char* cstr, spdlog::level::level_enum log_lvl);
#endif


protected:

	// Console ////////////////////////////////////////////////////////////////

	csys::System m_ConsoleSystem;            //!< Main console system.
	size_t m_HistoryIndex;                   //!< Command history index.

	// Dear ImGui  ////////////////////////////////////////////////////////////

	// Main

	std::string m_Buffer;            //!< Input buffer.
	std::string m_ConsoleName;       //!< Console name string buffer.
	ImGuiTextFilter m_TextFilter;    //!< Logging filer.
	bool m_AutoScroll;               //!< Auto scroll flag.
	bool m_ColoredOutput;            //!< Colored output flag.
	bool m_ScrollToBottom;           //!< Scroll to bottom after is command is ran
	bool m_FilterBar;                //!< Filter bar flag.
	bool m_TimeStamps;                 //!< Display time stamps flag
	bool m_WasOpen{ false };

	void InitIniSettings();             //!< Initialize Ini Settings handler
	void DefaultSettings();             //!< Restore console default settings
	void RegisterConsoleCommands();     //!< Register built-in console commands

	void MenuBar();                     //!< Console menu bar
	void FilterBar();                 //!< Console filter bar
	void InputBar();                 //!< Console input bar
	void LogWindow();                 //!< Console log

	static void HelpMaker(const char* desc);

	// Window appearance.

	float m_WindowAlpha;             //!< Window transparency

	enum COLOR_PALETTE
	{
		// This four have to match the csys item type enum.

		COL_COMMAND = 0,    //!< Color for command logs
		COL_LOG,            //!< Color for in-command logs
		COL_WARNING,        //!< Color for warnings logs
		COL_ERROR,          //!< Color for error logs
		COL_INFO,            //!< Color for info logs

		COL_TIMESTAMP,      //!< Color for timestamps

		COL_COUNT            //!< For bookkeeping purposes
	};

	std::array<ImVec4, COL_COUNT> m_ColorPalette;                //!< Container for all available colors

	// ImGui Console Window.

	static int InputCallback(ImGuiInputTextCallbackData* data);    //!< Console input callback
	bool m_WasPrevFrameTabCompletion = false;                    //!< Flag to determine if previous input was a tab completion
	// TODO(): popup with suggestions
	std::vector<std::string> m_CmdSuggestions;                    //!< Holds command suggestions from partial completion

#if 0
	// Save data inside .ini

	bool m_LoadedFromIni = false;

	static void SettingsHandler_ClearALl(ImGuiContext* ctx, ImGuiSettingsHandler* handler);

	static void SettingsHandler_ReadInit(ImGuiContext* ctx, ImGuiSettingsHandler* handler);

	static void* SettingsHandler_ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);

	static void SettingsHandler_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line);

	static void SettingsHandler_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler);

	static void SettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
	///////////////////////////////////////////////////////////////////////////
#endif
	private:
		PDIRECT3DTEXTURE9 m_texture_handle{ nullptr };
		int m_texture_width;
		int m_texture_height;
};
extern ImGuiConsole* console;	// statically initialized to an ImGuiConsoleLocal


template <typename Mutex> class ConsoleSink : public spdlog::sinks::base_sink<Mutex> {
public:

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted{};
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
		console->print(fmt::to_string(formatted).c_str(), msg.level);
	}

	void flush_() override {} // NOTE(): nothing to do

private:
};