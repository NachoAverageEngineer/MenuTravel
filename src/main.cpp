#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/utils/cocos.hpp>

#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/LevelSearchLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>

#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/CCTextInputNode.hpp>

using namespace geode::prelude;

namespace {
    static bool hasNodeOfTypeRecursive(cocos2d::CCNode* node, std::function<bool(cocos2d::CCNode*)> pred) {
        if (!node) return false;
        if (pred(node)) return true;

        auto children = node->getChildren();
        if (!children) return false;

        for (auto* child : CCArrayExt<cocos2d::CCNode*>(children)) {
            if (hasNodeOfTypeRecursive(child, pred)) return true;
        }
        return false;
    }

    static bool isInAttemptUnpaused() {
        if (PlayLayer::get() == nullptr) return false;

        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return true;

        auto scene = director->getRunningScene();
        if (!scene) return true;

        bool hasPause = hasNodeOfTypeRecursive(scene, [](cocos2d::CCNode* n) {
            return typeinfo_cast<PauseLayer*>(n) != nullptr;
            });

        return !hasPause;
    }

    static bool isTypingLikeUIIsOpen() {
        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return false;
        auto scene = director->getRunningScene();
        if (!scene) return false;

        return hasNodeOfTypeRecursive(scene, [](cocos2d::CCNode* n) {
            return typeinfo_cast<CCTextInputNode*>(n) != nullptr;
            });
    }

    static bool shouldBlock() {
        if (LevelEditorLayer::get() != nullptr) return true;
        if (isInAttemptUnpaused()) return true;
        if (isTypingLikeUIIsOpen()) return true;
        return false;
    }

    static void openSearchArea() {
        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto scene = LevelSearchLayer::scene(0);
        if (!scene) return;

        director->pushScene(scene);
    }

    static void openMainMenu() {
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

        level->m_levelID = id;

        auto scene = LevelInfoLayer::scene(level, false);
        if (!scene) return;

        director->replaceScene(scene);

        GameLevelManager::sharedState()->downloadLevel(id, false, 0);
    }

    static int getSlotId(int slot) {
        auto key = "level-id-" + std::to_string(slot);
        auto s = Mod::get()->getSettingValue<std::string>(key);
        try { return std::stoi(s); }
        catch (...) { return 0; }
    }

    static void handleKey(char const* settingId, bool down, bool repeat) {
        if (!down || repeat) return;
        if (shouldBlock()) return;

        std::string_view id = settingId;

        if (id == "open-search") {
            openSearchArea();
            return;
        }
        if (id == "open-main-menu") {
            openMainMenu();
            return;
        }

        constexpr std::string_view prefix = "open-level-";
        if (id.rfind(prefix, 0) == 0) {
            int slot = 0;
            try { slot = std::stoi(std::string(id.substr(prefix.size()))); }
            catch (...) { slot = 0; }
            if (slot < 1 || slot > 20) return;

            openLevelId(getSlotId(slot));
        }
    }
}

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