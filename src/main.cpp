#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/utils/cocos.hpp>

#include <Geode/binding/EditLevelLayer.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/LevelSearchLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>

#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/CCTextInputNode.hpp>

#include <cvolton.level-id-api/include/EditorIDs.hpp>

#include <cctype>
#include <climits>

using namespace geode::prelude;

namespace {
    static bool hasNodeOfTypeRecursive(
        cocos2d::CCNode* node,
        std::function<bool(cocos2d::CCNode*)> pred
    ) {
        if (!node) return false;
        if (pred(node)) return true;

        auto children = node->getChildren();
        if (!children) return false;

        for (auto* child : CCArrayExt<cocos2d::CCNode*>(children)) {
            if (hasNodeOfTypeRecursive(child, pred)) return true;
        }

        return false;
    }

    template <class T>
    static T* findNodeOfTypeRecursive(cocos2d::CCNode* node) {
        if (!node) return nullptr;

        if (auto casted = typeinfo_cast<T*>(node)) {
            return casted;
        }

        auto children = node->getChildren();
        if (!children) return nullptr;

        for (auto* child : CCArrayExt<cocos2d::CCNode*>(children)) {
            if (auto found = findNodeOfTypeRecursive<T>(child)) {
                return found;
            }
        }

        return nullptr;
    }

    static bool isInAttemptUnpaused() {
        if (PlayLayer::get() == nullptr) return false;

        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return true;

        auto scene = director->getRunningScene();
        if (!scene) return true;

        bool hasPause = hasNodeOfTypeRecursive(
            scene,
            [](cocos2d::CCNode* n) {
                return typeinfo_cast<PauseLayer*>(n) != nullptr;
            }
        );

        return !hasPause;
    }

    static bool isTypingLikeUIIsOpen() {
        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return false;

        auto scene = director->getRunningScene();
        if (!scene) return false;

        return hasNodeOfTypeRecursive(
            scene,
            [](cocos2d::CCNode* n) {
                return typeinfo_cast<CCTextInputNode*>(n) != nullptr;
            }
        );
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

    static std::string trim(std::string s) {
        auto isSpace = [](unsigned char c) {
            return std::isspace(c) != 0;
            };

        while (!s.empty() && isSpace((unsigned char)s.front())) {
            s.erase(s.begin());
        }

        while (!s.empty() && isSpace((unsigned char)s.back())) {
            s.pop_back();
        }

        return s;
    }

    static bool parseTravelTarget(
        std::string const& inputRaw,
        bool& outIsEditor,
        int& outId
    ) {
        auto input = trim(inputRaw);

        if (input.empty()) return false;

        outIsEditor = false;
        outId = 0;

        std::string lower = input;
        for (auto& ch : lower) {
            ch = (char)std::tolower((unsigned char)ch);
        }

        auto tryParseInt = [](std::string const& s, int& v) -> bool {
            try {
                size_t idx = 0;
                long long n = std::stoll(s, &idx, 10);

                if (idx != s.size()) return false;
                if (n <= 0 || n > INT_MAX) return false;

                v = (int)n;
                return true;
            }
            catch (...) {
                return false;
            }
            };

        if (lower.rfind("e:", 0) == 0) {
            outIsEditor = true;
            return tryParseInt(input.substr(2), outId);
        }

        if (lower.rfind("editor:", 0) == 0) {
            outIsEditor = true;
            return tryParseInt(input.substr(7), outId);
        }

        if (
            lower.size() >= 2 &&
            lower[0] == 'e' &&
            std::isdigit((unsigned char)lower[1])
            ) {
            outIsEditor = true;
            return tryParseInt(input.substr(1), outId);
        }

        outIsEditor = false;
        return tryParseInt(input, outId);
    }

    static bool shouldSearchOnlineLevelsInstead() {
        return Mod::get()->getSettingValue<bool>(
            "search-level-instead"
        );
    }

    static void searchOnlineLevelById(int id) {
        if (id <= 0) return;

        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto scene = LevelSearchLayer::scene(0);
        if (!scene) return;

        director->pushScene(scene);

        // Schedule on next frame so the scene has time to finish loading
        Loader::get()->queueInMainThread([id]() {
            auto director = cocos2d::CCDirector::sharedDirector();
            if (!director) return;

            auto runningScene = director->getRunningScene();
            if (!runningScene) return;

            auto searchLayer = findNodeOfTypeRecursive<LevelSearchLayer>(runningScene);
            if (!searchLayer) return;
            if (!searchLayer->m_searchInput) return;

            auto idString = std::to_string(id);
            searchLayer->m_searchInput->setString(idString.c_str());
            searchLayer->onSearch(nullptr);
            });
    }

    static void openEditorLevelByEditorId(int editorId) {
        if (editorId <= 0) return;

        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto level = EditorIDs::getLevelByID(editorId);
        if (!level) return;

        auto scene = EditLevelLayer::scene(level);
        if (!scene) return;

        director->replaceScene(scene);
    }

    static void openOnlineLevelDirectlyById(int id) {
        if (id <= 0) return;

        auto director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;

        auto level = GJGameLevel::create();
        if (!level) return;

        level->m_levelID = id;

        auto scene = LevelInfoLayer::scene(level, false);
        if (!scene) return;

        director->replaceScene(scene);

        if (auto manager = GameLevelManager::sharedState()) {
            manager->downloadLevel(id, false, 0);
        }
    }

    static void openOnlineLevelById(int id) {
        if (shouldSearchOnlineLevelsInstead()) {
            searchOnlineLevelById(id);
            return;
        }

        openOnlineLevelDirectlyById(id);
    }

    static std::string getSlotValue(int slot) {
        auto key = "level-id-" + std::to_string(slot);
        return Mod::get()->getSettingValue<std::string>(key);
    }

    static void openSlot(int slot) {
        bool isEditor = false;
        int id = 0;

        if (!parseTravelTarget(getSlotValue(slot), isEditor, id)) {
            return;
        }

        if (isEditor) {
            openEditorLevelByEditorId(id);
        }
        else {
            openOnlineLevelById(id);
        }
    }

    static void handleKey(
        char const* settingId,
        bool down,
        bool repeat
    ) {
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

            try {
                slot = std::stoi(
                    std::string(id.substr(prefix.size()))
                );
            }
            catch (...) {
                slot = 0;
            }

            if (slot < 1 || slot > 20) return;

            openSlot(slot);
        }
    }
}

$execute{
    auto bind = [](char const* id) {
        listenForKeybindSettingPresses(
            id,
            [id](
                Keybind const&,
                bool down,
                bool repeat,
                double
            ) {
                handleKey(id, down, repeat);
            }
        );
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