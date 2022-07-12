#pragma once

#include <array>
#include <vector>
#include "Config.hpp"

namespace utility
{
    class Input
    {
    public:
        void update();

        bool is_down(int vkey) const;
        bool went_down(int vkey) const;
        bool went_up(int vkey) const;

        const auto get_keys_size() const { return m_status.size(); };

    private:
        enum class InputState
        {
            IDLE,
            WENT_DOWN,
            WENT_UP
        };

        struct InputStatus
        {
            bool is_down = false;
            InputState state = InputState::IDLE;
        };

        std::array<InputStatus, 256> m_status;
    };


    struct Hotkey {

        Hotkey() = delete;
        Hotkey(const Hotkey& other) = delete;
        Hotkey(Hotkey&& other) = delete;

        // overload so you dont have to pass an std::vector<uint32_t> verbose shit for a 1 button
        Hotkey(uint32_t default_key, const char* description, const char* config_entry) : 
            m_default_keys(std::vector<uint32_t>{default_key}),
            m_description(description),
            m_config_entry(config_entry) {}
        // key combo one
        Hotkey(std::vector<uint32_t>& default_keys, const char* description, const char* config_entry) :
            m_default_keys(default_keys),
            m_description(description),
            m_config_entry(config_entry) {}

        std::vector<uint32_t> m_default_keys;

        std::string m_description;
        std::string m_config_entry;

        bool m_setting{ false };
        char m_tmp_buf[2]{0};

        // this could have been a fixed array but someone will complain
        // they cant bind adagio for strings and organ in G minor
        std::vector<uint32_t> m_vk_vec{};

        const char* hotkey_name(int hotkey); 
        void draw(utility::Input& input);
        bool check(utility::Input& input);

        void on_config_load(const utility::Config& cfg);
        void on_config_save(utility::Config& cfg);
    };

}