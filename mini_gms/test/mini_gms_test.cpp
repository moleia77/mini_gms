#include <iostream>
#include <string>
#include "mini_gms.hpp"

namespace mol::lib::test
{

    static int failures = 0;

    static void check(bool ok, const std::string &name)
    {
        if (ok)
        {
            std::cout << "[PASS] " << name << "\n";
        }
        else
        {
            std::cout << "[FAIL] " << name << "\n";
            failures++;
        }
    }

    // Test source
    static const char *SRC = R"(
# 1. Basic variables and batch flattening
[stu] {
    a : 1
    b : 2
    c { d : 3 }
    e : { 1 2 3 }
}

# 2. Macro reference &
[var] &base_hp : 100
[var] player_hp : &base_hp

# 3. Ranges &
[vec] frames : { 1~5 }
[vec] reverse_frames : { 5~1 }

# 4. Vec template inheritance
[vec] enemies : {
    { name : "Goblin"  hp : 100  alive : true }
    { "Slime"  50  false }
    { "Dragon"  5000  true }
}

# 5. Double and single quoted strings
[var] raw_str : "C:\Users\test"
[var] esc_str : 'line1\nline2'
[var] multi_line : '
    hello
    world
'

# 6. null
[var] nothing : null
)";

    int runAll()
    {
        failures = 0;

        auto doc = MgmsDoc::parse(SRC);

        // --- 1. Flattening and inference ---
        check(doc.get<mgms_num>("a") == 1.0, "batch flat: var a");
        check(doc.get<mgms_num>("b") == 2.0, "batch flat: var b");
        check(doc.get<mgms_num>("c.d") == 3.0, "batch flat: nested stu");
        check(doc.get<mgms_num>("e[0]") == 1.0, "batch flat: inferred vec");

        // --- 2. Macro references ---
        check(doc.get<mgms_num>("player_hp") == 100.0, "macro: player_hp");

        // --- 3. Ranges ---
        auto frames = doc.getVec("frames");
        check(frames.size() == 5, "range: 1~5 size");
        check(std::get<double>(frames[4]->asVar()) == 5.0, "range: 1~5 last");

        auto rev = doc.getVec("reverse_frames");
        check(rev.size() == 5, "range: 5~1 size");
        check(std::get<double>(rev[0]->asVar()) == 5.0, "range: 5~1 first");

        // --- 4. Vec template inheritance ---
        auto enemies = doc.getVec("enemies");
        check(enemies.size() == 3, "template: size");

        auto e0 = enemies[0]->asStu();
        auto e1 = enemies[1]->asStu();
        auto e2 = enemies[2]->asStu();

        check(e1[0].first == "name" && std::get<std::string>(e1[0].second->asVar()) == "Slime", "template: e1 name");
        check(e1[1].first == "hp" && std::get<double>(e1[1].second->asVar()) == 50.0, "template: e1 hp");
        check(e1[2].first == "alive" && std::get<bool>(e1[2].second->asVar()) == false, "template: e1 alive");

        // --- 5. Strings ---
        check(doc.get<mgms_str>("raw_str") == "C:\\Users\\test", "string: raw");
        check(doc.get<mgms_str>("esc_str") == "line1\nline2", "string: escaped");
        check(doc.get<mgms_str>("multi_line").find("hello") != std::string::npos, "string: multiline");

        // --- 6. null ---
        check(doc.get<mgms_str>("nothing", "default") == "default", "null: returns default");

        // --- Print result ---
        if (failures == 0)
        {
            std::cout << "\n[SUCCESS] All tests passed!\n";
            return 0;
        }
        else
        {
            std::cout << "\n[ERROR] " << failures << " tests failed!\n";
            return 1;
        }
    }
}

int main()
{
    return mol::lib::test::runAll();
}