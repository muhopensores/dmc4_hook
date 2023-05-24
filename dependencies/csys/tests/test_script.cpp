#include "doctest.h"
#include "csys/script.h"
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

TEST_CASE ("Test CSYS Script Class")
{
    using namespace csys;

    // Data.
    std::vector<std::string> script_expected_data = {"help", "run for help", "set time 10", "get time"};
    std::filesystem::path temp_script_path = std::filesystem::current_path();
    temp_script_path /= "temp_script.sh";

    // Create test script file.
    std::ofstream temp_script_file(temp_script_path);
    if (temp_script_file.is_open())
    {
        for (const auto &str: script_expected_data)
            temp_script_file << str << '\n';
        temp_script_file.close();
    }

    // Proper loading.
    Script script_(temp_script_path.string());
    auto check = [&]()
    {
        CHECK_MESSAGE(!script_.Data().empty(), "Script data is empty");
        CHECK_MESSAGE(script_.Data().size() == script_expected_data.size(), "Script size difference");
        for (size_t i = 0; i < script_expected_data.size(); ++i)
            CHECK_MESSAGE(script_.Data()[i] == script_expected_data[i], "Script data mismatch");
    };
    check();

    // Proper reloading.
    //temp_script_file.close();
    temp_script_file.clear();
    temp_script_file.open(temp_script_path, std::ios_base::app);
    if (temp_script_file.is_open())
    {
        temp_script_file << "AFTER_INITIAL_LOAD\n";
        temp_script_file.close();
    }
    script_expected_data.emplace_back("AFTER_INITIAL_LOAD");
    script_.Reload();
    check();

    // Remove temporal files.
    std::filesystem::remove(temp_script_path);
}