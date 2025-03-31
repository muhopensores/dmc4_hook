// Copyright (c) 2020 - present, Roland Munguia
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "Console.hpp"
#include "ModFramework.hpp"

#include <string>
#include "imgui_internal.h"
#include <cstring>
#include "utility/Dx9Utils.hpp"
#include "utility/String.hpp"
#include "utility/Compressed.hpp"
#include "misc/BgTexture.cpp"

#include <fstream>
#include <sstream>

static ImGuiConsole console_local("imgui-console", 256U);
ImGuiConsole* console = &console_local;

static float conspeed_float = 3.0f;

static int con_bg_red_int   = 0;
static int con_bg_green_int = 186;
static int con_bg_blue_int  = 250;
static int con_bg_alpha_int = 250;
bool ImGuiConsole::is_open = false;

// The following three functions (InputTextCallback_UserData, InputTextCallback, InputText) are obtained from misc/cpp/imgui_stdlib.h
// Which are licensed under MIT License (https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
namespace ImGui
{
	struct InputTextCallbackUserData
	{
		std::string* str;
		ImGuiInputTextCallback chain_callback;
		void* chain_callback_user_data;
	};

	static int input_text_callback(ImGuiInputTextCallbackData* data)
	{
		auto* user_data = (InputTextCallbackUserData*)data->UserData;
		if (data->EventChar < 256 && (data->EventChar == '`' || data->EventChar == '\'')) { // NOTE(): skip tilde / ' key press (its ' for me!)
			return 1;
		}
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			std::string* str = user_data->str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		}
		else if (user_data->chain_callback)
		{
			// Forward to user callback, if any
			data->UserData = user_data->chain_callback_user_data;
			return user_data->chain_callback(data);
		}
		return 0;
	}

	bool input_text(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallbackUserData cb_user_data;
		cb_user_data.str = str;
		cb_user_data.chain_callback = callback;
		cb_user_data.chain_callback_user_data = user_data;
		return InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, input_text_callback, &cb_user_data);
	}
}

ImGuiConsole::ImGuiConsole(std::string c_name, size_t input_buffer_size) : m_console_name(std::move(c_name))
{
	// Set input buffer size.
	m_buffer.resize(input_buffer_size);
	m_history_index = std::numeric_limits<size_t>::min();

	// Custom functions.
	register_console_commands();
}

void ImGuiConsole::draw(bool& open)
{
	is_open = open;
	if (!open && display_frac <= 0.001f ) {return; }

	// TODO(): block keyboard inputs -> done via including this in DisableKeyboard.cpp

	update_display_fraction();

	///////////////////////////////////////////////////////////////////////////
	// Window and Settings ////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	// Begin Console Window.
	ImGui::SetNextWindowBgAlpha(m_window_alpha);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, m_window_alpha));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, m_window_alpha));
	ImGui::Begin(m_console_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration);
	
	// bg texture
	if (m_texture_handle != nullptr) {
		auto* dl = ImGui::GetWindowDrawList();
		double texture_ar = 1024.0 / 576.0;
		glm::vec2 i_resolution(ImGui::GetWindowWidth() / 1024.0f, ImGui::GetWindowHeight() / 576.0f);
		glm::vec2 region_min = ImVec2(0.0f, 0.0f);
		glm::vec2 region_max = ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
		// imguing all over the floor
		dl->AddImage(m_texture_handle, region_min, region_max * i_resolution.x, ImVec2(0, 0), i_resolution, ImColor::ImColor(con_bg_red_int, con_bg_green_int, con_bg_blue_int, con_bg_alpha_int));
		// clockwise from top right
		//dl->AddQuadFilled(region_min, ImVec2(region_max.x, region_min.y), ImVec2(region_max.x, region_max.y), ImVec2(region_min.y, region_max.x), ImColor::ImColor(0.0f, 0.0f, 0.0f, 0.42f));
	}

	///////////////
	// Menu bar  //
	///////////////
	//menu_bar();

	////////////////
	// Filter bar //
	////////////////
	// if (m_filter_bar)
	// {
	//     filter_bar();
	// }

	//////////////////
	// Console Logs //
	//////////////////
	log_window();

	// Section off.
	ImGui::Separator();

	///////////////////////////////////////////////////////////////////////////
	// Command-line ///////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	input_bar();

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();

}

csys::System& ImGuiConsole::system()
{
	return m_console_system;
}

void ImGuiConsole::init_imgui()
{
#if 0
	// Specify custom data to be store/loaded from imgui.ini
	init_ini_settings();

	// Set Console ImGui default settings
	if (!m_loaded_from_ini)
	{
		default_settings();
	}
#else
	default_settings();
    load_texture();
#endif
}

void ImGuiConsole::on_reset() {
    spdlog::info("ImGuiConsole::on_reset()");
    if (m_texture_handle != nullptr) {
        m_texture_handle->Release();
        spdlog::info("ImGuiConsole::m_texture_handle->Release()");
        m_texture_handle = nullptr;
    }
}

void ImGuiConsole::load_texture() {
    spdlog::info("ImGuiConsole::load_texture()");
    auto [data, size] = utility::decompress_file_from_memory_base85(menu_bg_compressed_data_base85);
    if (!utility::dx9::load_texture_from_file(data, size, &m_texture_handle, &m_texture_width, &m_texture_height)) {
        spdlog::error("Failed to load console background texture");
        m_texture_handle = nullptr;
    }
    spdlog::info("ImGuiConsole::load_texture() -> texture loaded");
}

void ImGuiConsole::set_display_fraction(float frac)
{
	final_frac = frac;
	frac_time = g_framework->sys_ms();
}

void ImGuiConsole::update_display_fraction() {

	auto ms = g_framework->sys_ms();
	// scroll towards the destination height
	if (final_frac < display_frac)
	{
		display_frac -= conspeed_float * (ms - frac_time) * 0.001f;
		if (final_frac > display_frac)
		{
			display_frac = final_frac;
		}
		frac_time = ms;
	}
	else if (final_frac > display_frac)
	{
		display_frac += conspeed_float * (ms - frac_time) * 0.001f;
		if (final_frac < display_frac)
		{
			display_frac = final_frac;
		}
		frac_time = ms;
	}

	auto& io = ImGui::GetIO();
	ImVec2 new_pos = ImVec2(0.0f, (io.DisplaySize.y * display_frac) - (io.DisplaySize.y / 2.0f));
	ImVec2 new_size = ImVec2(io.DisplaySize.x, io.DisplaySize.y * final_frac);
	ImGui::SetNextWindowPos(new_pos);
	ImGui::SetNextWindowSize(new_size);
}

bool ImGuiConsole::dump_file(const std::string& filename) {
	std::ofstream f(utility::widen(filename));

	if (!f) {
		return false;
	}

	for (const auto& item : m_console_system.Items()) {
		
		if (item.m_Type == csys::ItemType::COMMAND) {
			continue;
		}

		f << item.Get();
	}

	return true;
}

void ImGuiConsole::print(const char* cstr, spdlog::level::level_enum log_lvl) {
	static constexpr csys::ItemType type[spdlog::level::level_enum::n_levels] = {
		csys::ItemType::LOG, csys::ItemType::LOG, csys::ItemType::LOG, csys::ItemType::WARNING,
		csys::ItemType::CSYS_ERROR, csys::ItemType::CSYS_ERROR, csys::ItemType::NONE };

	console->m_console_system.Log(type[log_lvl]) << cstr;
	console->m_scroll_to_bottom = true;
}

#if 0 // TODO(remove?)
void ImGuiConsole::init_ini_settings()
{

	ImGuiContext& g = *ImGui::GetCurrentContext();

	// Load from .ini
	if (g.Initialized && !g.SettingsLoaded && !m_loaded_from_ini)
	{
		ImGuiSettingsHandler console_ini_handler;
		console_ini_handler.TypeName = "imgui-console";
		console_ini_handler.TypeHash = ImHashStr("imgui-console");
		console_ini_handler.ClearAllFn = settings_handler_clear_all;
		console_ini_handler.ApplyAllFn = settings_handler_apply_all;
		console_ini_handler.ReadInitFn = settings_handler_read_init;
		console_ini_handler.ReadOpenFn = settings_handler_read_open;
		console_ini_handler.ReadLineFn = settings_handler_read_line;
		console_ini_handler.WriteAllFn = settings_handler_write_all;
		console_ini_handler.UserData = this;
		g.SettingsHandlers.push_back(console_ini_handler);
	}
	// else Ini settings already loaded!
}
#endif

void ImGuiConsole::default_settings()
{
	// Settings
	m_auto_scroll = true;
	m_scroll_to_bottom = false;
	m_colored_output = true;
	m_filter_bar = true;
	m_time_stamps = true;

	// Style
	m_window_alpha = 0.2f;
	m_color_palette[COL_COMMAND] = ImVec4(1.f, 1.f, 1.f, 1.f);
	//m_color_palette[COL_LOG] = ImVec4(1.f, 1.f, 1.f, 0.5f);
	m_color_palette[COL_LOG] = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
	m_color_palette[COL_WARNING] = ImVec4(1.0f, 0.87f, 0.37f, 1.f);
	m_color_palette[COL_ERROR] = ImVec4(1.f, 0.365f, 0.365f, 1.f);
	m_color_palette[COL_INFO] = ImVec4(0.46f, 0.96f, 0.46f, 1.f);
	m_color_palette[COL_TIMESTAMP] = ImVec4(1.f, 1.f, 1.f, 0.5f);
}

void ImGuiConsole::register_console_commands()
{
	m_console_system.RegisterCommand("clear", "Clear console log", [this]()
		{
			m_console_system.Items().clear();
		});

	m_console_system.RegisterCommand("filter", "Set screen filter", [this](const csys::String& filter)
		{
			// Reset filter buffer.
			std::memset(m_text_filter.InputBuf, '\0', 256);

			// Copy filter input buffer from client.
			std::copy(filter.m_String.c_str(), filter.m_String.c_str() + std::min(static_cast<int>(filter.m_String.length()), 255), m_text_filter.InputBuf);

			// Build text filter.
			m_text_filter.Build();

		}, csys::Arg<csys::String>("filter_str"));

	m_console_system.RegisterCommand("run", "Run given script", [this](const csys::String& filter)
		{
			// Logs command.
			m_console_system.RunScript(filter.m_String);
		}, csys::Arg<csys::String>("script_name"));

	
	m_console_system.RegisterCommand("quit", "Quit the application", [this]()
		{
			std::exit(EXIT_SUCCESS);
		});

	m_console_system.RegisterCommand("con_dump", "Dump contents of the console into a file", [this](const csys::String& filter)
		{
			dump_file(filter.m_String);
		}, csys::Arg<csys::String>("filename"));

	m_console_system.RegisterVariable("con_speed",    conspeed_float,   csys::Arg<float>("Float"));
	m_console_system.RegisterVariable("con_bg_alpha", con_bg_alpha_int, csys::Arg<int>("Int [0,255]"));
	m_console_system.RegisterVariable("con_bg_red",   con_bg_red_int,   csys::Arg<int>("Int [0,255]"));
	m_console_system.RegisterVariable("con_bg_green", con_bg_green_int, csys::Arg<int>("Int [0,255]"));
	m_console_system.RegisterVariable("con_bg_blue",  con_bg_blue_int,  csys::Arg<int>("Int [0,255]"));
}

void ImGuiConsole::filter_bar()
{
	m_text_filter.Draw("Filter", ImGui::GetWindowWidth() * 0.25f);
	ImGui::Separator();
}

void ImGuiConsole::log_window()
{
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollRegion##", ImVec2(0, -footer_height_to_reserve), false, 0))
	{
		// Display colored command output.
		static const float timestamp_width = ImGui::CalcTextSize("00:00:00:0000").x;    // Timestamp.
		int count = 0;                                                                       // Item count.

		// Wrap items.
		ImGui::PushTextWrapPos();

		// Display items.
		for (const auto& item : m_console_system.Items())
		{
			// Exit if word is filtered.
			if (!m_text_filter.PassFilter(item.Get().c_str()))
				continue;

			// Spacing between commands.
			if (item.m_Type == csys::COMMAND)
			{
				if (m_time_stamps) ImGui::PushTextWrapPos(ImGui::GetColumnWidth() - timestamp_width);    // Wrap before timestamps start.
				if (count++ != 0) ImGui::Dummy(ImVec2(-1, ImGui::GetFontSize()));                            // No space for the first command.
			}

			// Items.
			if (m_colored_output)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, m_color_palette[item.m_Type]);
				ImGui::TextUnformatted(item.Get().data());
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::TextUnformatted(item.Get().data());
			}


			// Time stamp.
			if (item.m_Type == csys::COMMAND && m_time_stamps)
			{
				// No wrap for timestamps
				ImGui::PopTextWrapPos();

				// Right align.
				ImGui::SameLine(ImGui::GetColumnWidth(-1) - timestamp_width);

				// Draw time stamp.
				ImGui::PushStyleColor(ImGuiCol_Text, m_color_palette[COL_TIMESTAMP]);
				ImGui::Text("%02d:%02d:%02d:%04d", ((item.m_TimeStamp / 1000 / 3600) % 24), ((item.m_TimeStamp / 1000 / 60) % 60),
					((item.m_TimeStamp / 1000) % 60), item.m_TimeStamp % 1000);
				ImGui::PopStyleColor();

			}
		}

		// Stop wrapping since we are done displaying console items.
		ImGui::PopTextWrapPos();

		// Auto-scroll logs.
		if ((m_scroll_to_bottom && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() || m_auto_scroll)))
			ImGui::SetScrollHereY(1.0f);
		m_scroll_to_bottom = false;

		// Loop through command string vector.
	}
		ImGui::EndChild();
}

void ImGuiConsole::input_bar()
{
	// Variables.
	ImGuiInputTextFlags input_text_flags =
		ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackCompletion |
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways;

	// Input widget. (Width an always fixed width)
	ImGui::PushItemWidth(-ImGui::GetStyle().ItemSpacing.x * 7);
	if (ImGui::input_text("Input", &m_buffer, input_text_flags, input_callback, this))
	{
		// Validate.
		if (!m_buffer.empty())
		{
			// Run command line input.
			m_console_system.RunCommand(m_buffer);

			// Scroll to bottom after its ran.
			m_scroll_to_bottom = true;
		}

		// Clear command line.
		m_buffer.clear();
	}
	ImGui::PopItemWidth();

	// Reset suggestions when client provides char input.
	if (ImGui::IsItemEdited() && !m_was_prev_frame_tab_completion)
	{
		m_cmd_suggestions.clear();
	}
	m_was_prev_frame_tab_completion = false;

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	ImGui::SetKeyboardFocusHere(-1);
}

void ImGuiConsole::menu_bar()
{
	if (ImGui::BeginMenuBar())
	{
		// Settings menu.
		if (ImGui::BeginMenu("Settings"))
		{
			// Colored output
			ImGui::Checkbox("Colored Output", &m_colored_output);
			ImGui::SameLine();
			HelpMaker("Enable colored command output");

			// Auto Scroll
			ImGui::Checkbox("Auto Scroll", &m_auto_scroll);
			ImGui::SameLine();
			HelpMaker("Automatically scroll to bottom of console log");

			// Filter bar
			ImGui::Checkbox("Filter Bar", &m_filter_bar);
			ImGui::SameLine();
			HelpMaker("Enable console filter bar");

			// Time stamp
			ImGui::Checkbox("Time Stamps", &m_time_stamps);
			ImGui::SameLine();
			HelpMaker("Display command execution timestamps");

			// Reset to default settings
			if (ImGui::Button("Reset settings", ImVec2(ImGui::GetColumnWidth(), 0)))
				ImGui::OpenPopup("Reset Settings?");

			// Confirmation
			if (ImGui::BeginPopupModal("Reset Settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("All settings will be reset to default.\nThis operation cannot be undone!\n\n");
				ImGui::Separator();

				if (ImGui::Button("Reset", ImVec2(120, 0)))
				{
					default_settings();
					ImGui::CloseCurrentPopup();
				}

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::EndMenu();
		}

		// View settings.
		if (ImGui::BeginMenu("Appearance"))
		{
			// Logging Colors
			ImGuiColorEditFlags flags =
				ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar;

			ImGui::TextUnformatted("Color Palette");
			ImGui::Indent();
			ImGui::ColorEdit4("Command##", (float*)&m_color_palette[COL_COMMAND], flags);
			ImGui::ColorEdit4("Log##", (float*)&m_color_palette[COL_LOG], flags);
			ImGui::ColorEdit4("Warning##", (float*)&m_color_palette[COL_WARNING], flags);
			ImGui::ColorEdit4("Error##", (float*)&m_color_palette[COL_ERROR], flags);
			ImGui::ColorEdit4("Info##", (float*)&m_color_palette[COL_INFO], flags);
			ImGui::ColorEdit4("Time Stamp##", (float*)&m_color_palette[COL_TIMESTAMP], flags);
			ImGui::Unindent();

			ImGui::Separator();

			// Window transparency.
			ImGui::TextUnformatted("Background");
			ImGui::SliderFloat("Transparency##", &m_window_alpha, 0.1f, 1.f);

			ImGui::EndMenu();
		}

		// All scripts.
		if (ImGui::BeginMenu("Scripts"))
		{
			// Show registered scripts.
			for (const auto& scr_pair : m_console_system.Scripts())
			{
				if (ImGui::MenuItem(scr_pair.first.c_str()))
				{
					m_console_system.RunScript(scr_pair.first);
					m_scroll_to_bottom = true;
				}
			}

			// Reload scripts.
			ImGui::Separator();
			if (ImGui::Button("Reload Scripts", ImVec2(ImGui::GetColumnWidth(), 0)))
			{
				for (const auto& scr_pair : m_console_system.Scripts())
				{
					scr_pair.second->Reload();
				}
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

// From imgui_demo.cpp
void ImGuiConsole::HelpMaker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

int ImGuiConsole::input_callback(ImGuiInputTextCallbackData* data)
{

	// Exit if no buffer.
	if (data->BufTextLen == 0 && (data->EventFlag != ImGuiInputTextFlags_CallbackHistory))
		return 0;

	// Get input string and console.
	std::string input_str = data->Buf;
	std::string trim_str;
	auto console = static_cast<ImGuiConsole*>(data->UserData);

	// Optimize by only using positions.
	// Trim start and end spaces.
	size_t start_pos = console->m_buffer.find_first_not_of(' ');
	size_t end_pos = console->m_buffer.find_last_not_of(' ');

	// Get trimmed string.
	if (start_pos != std::string::npos && end_pos != std::string::npos)
		trim_str = console->m_buffer.substr(start_pos, end_pos + 1);
	else
		trim_str = console->m_buffer;

	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// Find last word.
		size_t start_subtr_pos = trim_str.find_last_of(' ');
		csys::AutoComplete* console_autocomplete;

		// Command line is an entire word/string (No whitespace)
		// Determine which autocomplete tree to use.
		if (start_subtr_pos == std::string::npos)
		{
			start_subtr_pos = 0;
			console_autocomplete = &console->m_console_system.CmdAutocomplete();
		}
		else
		{
			start_subtr_pos += 1;
			console_autocomplete = &console->m_console_system.VarAutocomplete();
		}

		// Validate str
		if (!trim_str.empty())
		{
			// Display suggestions on console.
			if (!console->m_cmd_suggestions.empty())
			{
				console->m_console_system.Log(csys::COMMAND) << "Suggestions: " << csys::endl;

				for (const auto& suggestion : console->m_cmd_suggestions)
					console->m_console_system.Log(csys::LOG) << suggestion << csys::endl;

				console->m_cmd_suggestions.clear();
			}

			// Get partial completion and suggestions.
			std::string partial = console_autocomplete->Suggestions(trim_str.substr(start_subtr_pos, end_pos + 1), console->m_cmd_suggestions);

			// Autocomplete only when one work is available.
			if (!console->m_cmd_suggestions.empty() && console->m_cmd_suggestions.size() == 1)
			{
				data->DeleteChars(static_cast<int>(start_subtr_pos), static_cast<int>(data->BufTextLen - start_subtr_pos));
				data->InsertChars(static_cast<int>(start_subtr_pos), console->m_cmd_suggestions[0].data());
				console->m_cmd_suggestions.clear();
			}
			else
			{
				// Partially complete word.
				if (!partial.empty())
				{
					data->DeleteChars(static_cast<int>(start_subtr_pos), static_cast<int>(data->BufTextLen - start_subtr_pos));
					data->InsertChars(static_cast<int>(start_subtr_pos), partial.data());
				}
			}
		}

		// We have performed the completion event.
		console->m_was_prev_frame_tab_completion = true;
	}
	break;

	case ImGuiInputTextFlags_CallbackHistory:
	{
		// Clear buffer.
		data->DeleteChars(0, data->BufTextLen);

		// Init history index
		if (console->m_history_index == std::numeric_limits<size_t>::min())
			console->m_history_index = console->m_console_system.History().GetNewIndex();

		// Traverse history.
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (console->m_history_index) --(console->m_history_index);
		}
		else
		{
			if (console->m_history_index < console->m_console_system.History().Size()) ++(console->m_history_index);
		}

		// Get history.
		std::string prev_command = console->m_console_system.History()[console->m_history_index];

		// Insert commands.
		data->InsertChars(data->CursorPos, prev_command.data());
	}
	break;

	case ImGuiInputTextFlags_CallbackCharFilter:
	case ImGuiInputTextFlags_CallbackAlways:
	default:
		break;
	}
	return 0;
}

#if 0 // TODO(): we dont really need those i think
void ImGuiConsole::settings_handler_clear_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler)
{
}

void ImGuiConsole::settings_handler_read_init(ImGuiContext* ctx, ImGuiSettingsHandler* handler)
{
}

void* ImGuiConsole::settings_handler_read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
{
	if (!handler->UserData)
		return nullptr;

	auto console = static_cast<ImGuiConsole*>(handler->UserData);

	if (strcmp(name, console->m_console_name.c_str()) != 0)
		return nullptr;
	return (void*)1;
}

void ImGuiConsole::settings_handler_read_line(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
{
	if (!handler->UserData)
		return;

	// Get console.
	auto console = static_cast<ImGuiConsole*>(handler->UserData);

	// Ensure console doesn't reset variables.
	console->m_loaded_from_ini = true;

	// Disable warning regarding sscanf when using MVSC
#pragma warning( push )
#pragma warning( disable:4996 )

#define INI_CONSOLE_LOAD_COLOR(type) (std::sscanf(line, #type"=%i,%i,%i,%i", &r, &g, &b, &a) == 4) { console->m_color_palette[type] = ImColor(r, g, b, a); }
#define INI_CONSOLE_LOAD_FLOAT(var) (std::sscanf(line, #var"=%f", &f) == 1) { console->var = f; }
#define INI_CONSOLE_LOAD_BOOL(var) (std::sscanf(line, #var"=%i", &b) == 1) {console->var = b == 1;}

	float f;
	int r, g, b, a;

	// Window style/visuals
	if INI_CONSOLE_LOAD_COLOR(COL_COMMAND)
	else if INI_CONSOLE_LOAD_COLOR(COL_LOG)
	else if INI_CONSOLE_LOAD_COLOR(COL_WARNING)
	else if INI_CONSOLE_LOAD_COLOR(COL_ERROR)
	else if INI_CONSOLE_LOAD_COLOR(COL_INFO)
	else if INI_CONSOLE_LOAD_COLOR(COL_TIMESTAMP)
	else if INI_CONSOLE_LOAD_FLOAT(m_window_alpha)

		// Window settings
	else if INI_CONSOLE_LOAD_BOOL(m_auto_scroll)
	else if INI_CONSOLE_LOAD_BOOL(m_scroll_to_bottom)
	else if INI_CONSOLE_LOAD_BOOL(m_colored_output)
	else if INI_CONSOLE_LOAD_BOOL(m_filter_bar)
	else if INI_CONSOLE_LOAD_BOOL(m_time_stamps)

#pragma warning( pop )
}

void ImGuiConsole::settings_handler_apply_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler)
{
	if (!handler->UserData)
		return;
}

void ImGuiConsole::settings_handler_write_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
	if (!handler->UserData)
		return;

	// Get console.
	auto console = static_cast<ImGuiConsole*>(handler->UserData);

#define INI_CONSOLE_SAVE_COLOR(type) buf->appendf(#type"=%i,%i,%i,%i\n", (int)(console->m_color_palette[type].x * 255),\
                                                                         (int)(console->m_color_palette[type].y * 255),\
                                                                         (int)(console->m_color_palette[type].z * 255),\
                                                                         (int)(console->m_color_palette[type].w * 255))

#define INI_CONSOLE_SAVE_FLOAT(var) buf->appendf(#var"=%.3f\n", console->var)
#define INI_CONSOLE_SAVE_BOOL(var) buf->appendf(#var"=%i\n", console->var)

	// Set header for CONSOLE Console.
	buf->appendf("[%s][%s]\n", handler->TypeName, console->m_console_name.data());

	// Window settings.
	INI_CONSOLE_SAVE_BOOL(m_auto_scroll);
	INI_CONSOLE_SAVE_BOOL(m_scroll_to_bottom);
	INI_CONSOLE_SAVE_BOOL(m_colored_output);
	INI_CONSOLE_SAVE_BOOL(m_filter_bar);
	INI_CONSOLE_SAVE_BOOL(m_time_stamps);

	// Window style/visuals
	INI_CONSOLE_SAVE_FLOAT(m_window_alpha);
	INI_CONSOLE_SAVE_COLOR(COL_COMMAND);
	INI_CONSOLE_SAVE_COLOR(COL_LOG);
	INI_CONSOLE_SAVE_COLOR(COL_WARNING);
	INI_CONSOLE_SAVE_COLOR(COL_ERROR);
	INI_CONSOLE_SAVE_COLOR(COL_INFO);
	INI_CONSOLE_SAVE_COLOR(COL_TIMESTAMP);

	// End saving.
	buf->append("\n");
}
#endif
