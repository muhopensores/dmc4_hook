#include "doctest.h"
#include "csys/item.h"

TEST_CASE ("Autocomplete")
{
    // Testing none.
    SUBCASE("Testing Command Item Types + Basic functionality")
    {
        // LOG
        auto temp = csys::Item();
        CHECK(temp.m_Type == csys::ItemType::LOG);
        CHECK(temp.Get() == "\t");
        CHECK(temp.m_Data.empty());

        // Command
        temp = csys::Item(csys::ItemType::COMMAND);
        CHECK(temp.m_Type == csys::ItemType::COMMAND);
        CHECK(temp.Get() == "> ");
        CHECK(temp.m_Data.empty());

        // Warning
        temp = csys::Item(csys::ItemType::WARNING);
        CHECK(temp.m_Type == csys::ItemType::WARNING);
        CHECK(temp.Get() == "\t[WARNING]: ");
        CHECK(temp.m_Data.empty());

        // Error
        temp = csys::Item(csys::ItemType::ERROR);
        CHECK(temp.m_Type == csys::ItemType::ERROR);
        CHECK(temp.Get() == "[ERROR]: ");
        CHECK(temp.m_Data.empty());

        // Info
        temp = csys::Item(csys::ItemType::INFO) << "Test";
        CHECK(temp.m_Type == csys::ItemType::INFO);
        CHECK(temp.Get() == "Test");

        // None
        temp = csys::Item(csys::ItemType::NONE);
        CHECK(temp.m_Type == csys::ItemType::NONE);
        CHECK(temp.Get() == "");
        CHECK(temp.m_Data.empty());
    }

    SUBCASE("Testing Item Log")
    {
        // Logging an item.
        csys::ItemLog temp;
        temp.log(csys::LOG);
        CHECK(temp.Items().size() == 1);
        temp.Clear();
        CHECK(temp.Items().size() == 0);
    }
}

