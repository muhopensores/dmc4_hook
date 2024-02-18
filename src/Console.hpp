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
	void draw(bool& open);

	/*!
	 * \brief Console system which handles the console functionality (Logging, Commands, History, Scripts, etc).
	 * \return System Obj
	 */
	csys::System& system();

	// could be done in constructor but we need to defer till imgui init :(
	void init_imgui();
    void on_reset();
    void load_texture();

	float display_frac;	             // approaches final_frac at con_speed
	float final_frac;	             // 0.0 to 1.0 lines of console to display
	int	  frac_time;		         // time of last display_frac update

	void set_display_fraction(float frac);
	void update_display_fraction();

	bool dump_file(const std::string& filename);

#if 0
	template <typename... Param> static void print(std::string_view format, Param && ... param) {
		console->m_console_system.Log() << fmt::format(format, std::forward<Param>(param)...);
	}
#else
	void print(const char* cstr, spdlog::level::level_enum log_lvl);
#endif


protected:

	// Console ////////////////////////////////////////////////////////////////

	csys::System m_console_system;            //!< Main console system.
	size_t m_history_index;                   //!< Command history index.

	// Dear ImGui  ////////////////////////////////////////////////////////////

	// Main

	std::string m_buffer;            //!< Input buffer.
	std::string m_console_name;       //!< Console name string buffer.
	ImGuiTextFilter m_text_filter;    //!< Logging filer.
	bool m_auto_scroll;               //!< Auto scroll flag.
	bool m_colored_output;            //!< Colored output flag.
	bool m_scroll_to_bottom;           //!< Scroll to bottom after is command is ran
	bool m_filter_bar;                //!< Filter bar flag.
	bool m_time_stamps;                 //!< Display time stamps flag
	bool m_was_open{ false };

	void init_ini_settings();             //!< Initialize Ini Settings handler
	void default_settings();             //!< Restore console default settings
	void register_console_commands();     //!< Register built-in console commands

	void menu_bar();                     //!< Console menu bar
	void filter_bar();                 //!< Console filter bar
	void input_bar();                 //!< Console input bar
	void log_window();                 //!< Console log

	static void HelpMaker(const char* desc);

	// Window appearance.

	float m_window_alpha;             //!< Window transparency

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

	std::array<ImVec4, COL_COUNT> m_color_palette;                //!< Container for all available colors

	// ImGui Console Window.

	static int input_callback(ImGuiInputTextCallbackData* data);    //!< Console input callback
	bool m_was_prev_frame_tab_completion = false;                    //!< Flag to determine if previous input was a tab completion
	// TODO(): popup with suggestions
	std::vector<std::string> m_cmd_suggestions;                    //!< Holds command suggestions from partial completion

#if 0
	// Save data inside .ini

	bool m_loaded_from_ini = false;

	static void settings_handler_clear_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler);

	static void settings_handler_read_init(ImGuiContext* ctx, ImGuiSettingsHandler* handler);

	static void* settings_handler_read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);

	static void settings_handler_read_line(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line);

	static void settings_handler_apply_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler);

	static void settings_handler_write_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
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