#include "MessageDisplay.hpp"
#include <mutex>
#if 0
#include "spdlog/sinks/base_sink.h"

template<typename Mutex>
class imgui_message_sink final : public base_sink<Mutex>
{
    imgui_message_sink();
    ~imgui_message_sink() override = default;

protected:
    void sink_it_(const details::log_msg &msg) override {
        memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        DISPLAY_MESSAGE(fmt::to_string(formatted));
    };
    void flush_() override {};
};

#include "spdlog/details/null_mutex.h"
#include <mutex>
using imgui_message_sink_mt = imgui_message_sink<std::mutex>;
using imgui_message_sink_st = imgui_message_sink<spdlog::details::null_mutex>;

#endif

std::mutex g_msg_mutex;

bool Message::update(float dt) {
    if (!this) { return false; }
	m_time -= dt;
	if (m_time <= 0.0f) {
		return false;
	}
    ImGui::TextWrapped("%s",m_message.c_str());
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    ImGui::GetBackgroundDrawList()->AddRectFilled(min, max, IM_COL32(64, 64, 64, 128));
	return true;
}

void MessageDisplay::add_message(std::string msg) {
    const std::lock_guard<std::mutex> lock(g_msg_mutex);
	m_messages.push_front(std::move(msg));
}

/*void MessageDisplay::show_messages()  {
	for (Message& m : m_messages) {
		m.show();
	}
}*/

void MessageDisplay::update_messages() {
    bool pop = false;
    for (Message& m : m_messages) {
        if (!m.update(0.1f)) {
            pop = true;
        }
    }
    if (pop) {
        const std::lock_guard<std::mutex> lock(g_msg_mutex);
        m_messages.pop_back();
    }
}