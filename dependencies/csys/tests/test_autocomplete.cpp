#include <sstream> // Fix for VS2019
#include "doctest.h"
#include "csys/autocomplete.h"
#include <algorithm>
#include <string>
#include <vector>

#define SEARCH_CHECK(word){\
    CHECK(tree.Search(word));}\

#define SUGGESTION_CHECK(tree, prefix, ...)\
    {\
        auto suggestions = std::vector<std::string>({__VA_ARGS__});\
        auto results = tree.Suggestions(prefix);\
        bool check = false;\
        for (const auto &result : *results)\
        {\
            for (const auto &suggestion : suggestions)\
            {\
                if (suggestion == result)\
                    check = true;\
            }\
        }\
        CHECK(check);\
    }\

#define SUGGESTION_PARTIAL_CHECK(tree, prefix, partial_ac, ...)\
    {\
        auto suggestions = std::vector<std::string>({__VA_ARGS__});\
        std::vector<std::string> results;\
        std::string partial = prefix;\
        tree.Suggestions(partial, results, true);\
        bool check = false;\
        for (const auto &suggestion : suggestions)\
        {\
            check = std::find(results.begin(), results.end(), suggestion.c_str()) != results.end();\
        }\
        bool check_2 = partial == partial_ac;\
        CHECK_MESSAGE(suggestions.size() == results.size(), "Size not matched");\
        CHECK_MESSAGE(check, "Autocomplete suggestions did not match expected output");\
        CHECK_MESSAGE(check_2, std::string("Partial completion did not match expected output -> ") + partial + " != " + partial_ac);\
    }

TEST_CASE ("Autocomplete")
{
    csys::AutoComplete tree({"roland", "munguia", "12345", "michael", "rino", "muchos"});

    // Word search.
    SUBCASE("Searching for word completion")
    {
        SEARCH_CHECK("roland")
        SEARCH_CHECK("munguia")
        SEARCH_CHECK("12345")
        SEARCH_CHECK("michael")
        SEARCH_CHECK("rino")
        SEARCH_CHECK("muchos")
    }

    // Completion.
    SUBCASE("Searching for auto complete")
    {
        SUGGESTION_CHECK(tree, "r", "roland", "rino")
        SUGGESTION_CHECK(tree, "ro", "roland")
        SUGGESTION_CHECK(tree, "ri", "rino")
        SUGGESTION_CHECK(tree, "1", "12345")
        SUGGESTION_CHECK(tree, "m", "munguia", "michael")
        SUGGESTION_CHECK(tree, "mu", "munguia", "muchos")
    }

    // Insertion.
    SUBCASE("Inserting + Searching for autocomplete")
    {
        SUGGESTION_CHECK(tree, "r", "roland", "rino")
        SUGGESTION_CHECK(tree, "ro", "roland")
        CHECK(tree.Count() == 6);
        CHECK(tree.Size() == 31);
        tree.Insert("rolling");
        SUGGESTION_CHECK(tree, "r", "roland", "rino", "rolling")
        SUGGESTION_CHECK(tree, "ro", "roland", "rolling")
        CHECK(tree.Size() == 35);
        CHECK(tree.Count() == 7);
        tree.Insert("rolling");
        CHECK(tree.Size() == 35);
        CHECK(tree.Count() == 7);
    }

    // Partial completion
    csys::AutoComplete tree2({"roland", "munguia", "12345", "michael", "rolling", "muchos", "rolipoli"});
    SUBCASE("Searching for partial complete + suggestions")
    {
        SUGGESTION_PARTIAL_CHECK(tree2, "r", "rol", "roland", "rolipoli", "rolling")
        SUGGESTION_PARTIAL_CHECK(tree2, "m", "m", "munguia", "muchos", "michael")
    }

    // Deletion from tree.
    SUBCASE("Delete notes from tree")
    {
        tree2.Insert("roli");
        CHECK(tree2.Search("roland"));
        tree2.Remove("roland");
        CHECK(tree2.Search("roli"));
        tree2.Remove("roli");
        CHECK(!tree2.Search("roli"));
        CHECK(!tree2.Search("roland"));
        CHECK(tree2.Search("rolling"));
        CHECK(tree2.Search("rolipoli"));
        SUGGESTION_PARTIAL_CHECK(tree2, "r", "rol", "rolipoli", "rolling");
    }

    // Copying tree.
    SUBCASE("Copying trees")
    {
        auto cTree(tree);
        CHECK(cTree.Search("roland"));
        CHECK(cTree.Search("munguia"));
        CHECK(cTree.Search("12345"));
        CHECK(cTree.Search("michael"));
        CHECK(cTree.Search("rino"));
        CHECK(cTree.Search("muchos"));

        cTree.Remove("roland");
        cTree.Remove("munguia");

        csys::AutoComplete aTree;
        aTree = cTree;
        CHECK(!aTree.Search("roland"));
        CHECK(!aTree.Search("munguia"));
        CHECK(aTree.Search("12345"));
        CHECK(aTree.Search("michael"));
        CHECK(aTree.Search("rino"));
        CHECK(aTree.Search("muchos"));
    }
}

