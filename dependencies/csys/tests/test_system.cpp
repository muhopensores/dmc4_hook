#include "doctest.h"
#include "csys/system.h"

void setter(float& v, const float &r) { v = r; }

TEST_CASE ("Test CSYS System Class")
{
    csys::System temp;

    bool test_flag = false;
    auto fn = [&](bool flag)
    { test_flag = flag; };
    float time_variable = 0;
    int temp_var = 0;

    // Test Command registration.
    temp.RegisterCommand("test", "Simple description", fn, csys::Arg<bool>("Test_Argument"));
    temp.RunCommand("test true");
    CHECK(test_flag);

    bool runs = false;
    temp.RegisterCommand("empty", "Function that takes no params", [&runs](){ runs = true; });
    temp.RunCommand("empty");
    CHECK(runs);

    // Registration.
    temp.RegisterVariable("time", time_variable, csys::Arg<float>(""));
    temp.RegisterVariable("temp_var", temp_var, csys::Arg<int>(""));

    temp.RegisterVariable("time_set", time_variable, setter);

    temp.RunCommand("set time_set 30");
    CHECK(time_variable == 30);

    // Modification.
    temp.RunCommand("set time 10");
    CHECK(time_variable == 10);
    temp.RunCommand("set time 15");
    CHECK(time_variable == 15);
    temp.RunCommand("set temp_var 30");
    CHECK(temp_var == 30);

    // Test system variables.
    temp.UnregisterVariable("time");
    temp.RunCommand("set time 10");
    CHECK(time_variable == 15);

    // Test system commands.
    temp.UnregisterCommand("test");
    temp.RunCommand("test false");
    CHECK(test_flag);
}