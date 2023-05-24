#include "doctest.h"
#include "csys/history.h"

TEST_CASE ("Test csys history ")
{
    // Check insertion.
    SUBCASE("Testing command history insertion")
    {
        csys::CommandHistory c_history;
        std::vector<std::string> commands({"h1", "h2", "h3", "h4"});

        for (const auto &cmd : commands)
        {
            c_history.PushBack(cmd);
        }

        for (size_t i = 0, size = commands.size(); i < size; ++i)
        {
            CHECK(c_history[i] == commands[i]);
        }

        CHECK(c_history.GetNew() == commands.back());
    }

    // Check looping.
    SUBCASE("Check history looping")
    {
        csys::CommandHistory c_history(3);
        std::vector<std::string> commands({"h1", "h2", "h3", "h4"});

        for (const auto &cmd : commands)
        {
            c_history.PushBack(cmd);
            CHECK(c_history.GetNew() == cmd);
        }

        CHECK(c_history[0] == commands[3]);
        CHECK(c_history[1] == commands[1]);
        CHECK(c_history[2] == commands[2]);

        CHECK(c_history.GetOld() == "h2");
        CHECK(c_history.GetOldIndex() == 1);
        CHECK(c_history.GetNew() == commands.back());
        CHECK(c_history.Size() == 3);

        c_history.Clear();
        for (unsigned int i = 0; i < 3; ++i)
            c_history.PushBack(commands[i]);
        CHECK(c_history.GetOld() == "h1");
        CHECK(c_history.GetOldIndex() == 0);
        CHECK(c_history.GetNew() == "h3");
    }

    // Check clearing.
    SUBCASE("Check history clearing")
    {
        csys::CommandHistory c_history(64);
        std::vector<std::string> commands({"h1", "h2", "h3", "h4"});

        for (const auto &cmd : commands)
        {
            c_history.PushBack(cmd);
        }

        CHECK(c_history.Size() == 4);

        for (int i = 0; i < (int) c_history.Capacity(); ++i)
            c_history.PushBack("dummy" + std::to_string(i));

        CHECK(c_history.Size() == c_history.Capacity());

        CHECK(c_history.GetNew() == "dummy63");
        CHECK(c_history.GetNewIndex() == 3);
        CHECK(c_history.GetOldIndex() == 4);

        c_history.Clear();
        CHECK(c_history.Size() == 0);
    }
}