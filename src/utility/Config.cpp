#include <fstream>
#include <sstream>

#include "String.hpp"
#include "Config.hpp"

using namespace std;

namespace utility {
    Config::Config(const string& file_path)
        : m_key_values{}
    {
        if (!file_path.empty()) {
            load(file_path);
        }
    }

    bool Config::load(const string& file_path) {
        if (file_path.empty()) {
            return false;
        }

        ifstream f(widen(file_path));

        if (!f) {
            return false;
        }

        for (string line{}; getline(f, line); ) {
            istringstream ss{ line };
            string key{};
            string value{};

            getline(ss, key, '=');
            getline(ss, value);

            set(key, value);
        }

        return true;
    }

    bool Config::save(const string& file_path) {
        ofstream f(widen(file_path));

        if (!f) {
            return false;
        }

        for (auto& key_value : m_key_values) {
            f << key_value.first << "=" << key_value.second << endl;
        }

        return true;
    }

    optional<string> Config::get(const string& key) const {
        auto search = m_key_values.find(key);

        if (search == m_key_values.end()) {
            return {};
        }

        return search->second;
    }

    void Config::set(const string& key, const string& value) {
        if (!key.empty() && !value.empty()) {
            m_key_values[key] = value;
        }
    }
}
