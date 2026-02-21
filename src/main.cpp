#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

#include <Geode/cocos/extensions/cocos-ext.h>

#include <Geode/binding/GameLevelManager.hpp>

#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/LevelSearchLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>

using namespace geode::prelude;

namespace {

    // OG: open search area
    void openSearchArea() {
        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto scene = LevelSearchLayer::scene(0);
        if (!scene) return;

        director->pushScene(scene);
    }

    // main menu
    void openMainMenu() {
        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto scene = MenuLayer::scene(false);
        if (!scene) return;

        director->replaceScene(scene);
    }

    static void openLevelId(int id) {
        if (id <= 0) return;

        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto level = GJGameLevel::create();
        if (!level) return;

        // set the id (some bindings have setLevelID, some use m_levelID)
        level->m_levelID = id;

        // open the page
        auto scene = LevelInfoLayer::scene(level, false);
        if (!scene) return;
        director->replaceScene(scene);

        // fetch metadata so name/creator/song appear
        // your binding expects 3 args
        // (id, isGauntlet=false, extra=0)
        GameLevelManager::sharedState()->downloadLevel(id, false, 0);
    }

    int getSlotId(int slot) {
        // settings keys: level-id-1 ... level-id-20
        std::string key = "level-id-" + std::to_string(slot);
        auto s = Mod::get()->getSettingValue<std::string>(key);

        try { return std::stoi(s); }
        catch (...) { return 0; }
    }

    void handleKey(char const* settingId, bool down, bool repeat) {
        if (!down || repeat) return;

        std::string_view id = settingId;

        if (id == "open-search") {
            openSearchArea();
            return;
        }
        if (id == "open-main-menu") {
            openMainMenu();
            return;
        }

        // open-level-1 .. open-level-20
        constexpr std::string_view prefix = "open-level-";
        if (id.rfind(prefix, 0) == 0) {
            int slot = 0;
            try {
                slot = std::stoi(std::string(id.substr(prefix.size())));
            }
            catch (...) {
                slot = 0;
            }
            if (slot < 1 || slot > 20) return;

            openLevelId(getSlotId(slot));
            return;
        }
    }

} // namespace

$execute{
    auto bind = [](char const* id) {
        listenForKeybindSettingPresses(id, [id](Keybind const&, bool down, bool repeat, double) {
            handleKey(id, down, repeat);
        });
    };

    bind("open-search");
    bind("open-main-menu");

    bind("open-level-1");
    bind("open-level-2");
    bind("open-level-3");
    bind("open-level-4");
    bind("open-level-5");
    bind("open-level-6");
    bind("open-level-7");
    bind("open-level-8");
    bind("open-level-9");
    bind("open-level-10");
    bind("open-level-11");
    bind("open-level-12");
    bind("open-level-13");
    bind("open-level-14");
    bind("open-level-15");
    bind("open-level-16");
    bind("open-level-17");
    bind("open-level-18");
    bind("open-level-19");
    bind("open-level-20");
}