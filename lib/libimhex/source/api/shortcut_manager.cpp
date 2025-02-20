#include <hex/api/shortcut_manager.hpp>
#include <imgui.h>
#include <hex/api/content_registry.hpp>

#include <hex/ui/view.hpp>

namespace hex {

    namespace {

        std::map<Shortcut, ShortcutManager::ShortcutEntry> s_globalShortcuts;
        std::atomic<bool> s_paused;
        std::optional<Shortcut> s_prevShortcut;

    }


    void ShortcutManager::addGlobalShortcut(const Shortcut &shortcut, const std::string &unlocalizedName, const std::function<void()> &callback) {
        s_globalShortcuts.insert({ shortcut, { shortcut, unlocalizedName, callback } });
    }

    void ShortcutManager::addShortcut(View *view, const Shortcut &shortcut, const std::string &unlocalizedName, const std::function<void()> &callback) {
        view->m_shortcuts.insert({ shortcut + CurrentView, { shortcut, unlocalizedName, callback } });
    }

    static Shortcut getShortcut(bool ctrl, bool alt, bool shift, bool super, bool focused, u32 keyCode) {
        Shortcut pressedShortcut;

        if (ctrl)
            pressedShortcut += CTRL;
        if (alt)
            pressedShortcut += ALT;
        if (shift)
            pressedShortcut += SHIFT;
        if (super)
            pressedShortcut += SUPER;
        if (focused)
            pressedShortcut += CurrentView;

        pressedShortcut += static_cast<Keys>(keyCode);

        return pressedShortcut;
    }

    static void processShortcut(const Shortcut &shortcut, const std::map<Shortcut, ShortcutManager::ShortcutEntry> &shortcuts) {
        if (s_paused) return;

        if (ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopupId))
            return;

        if (shortcuts.contains(shortcut + AllowWhileTyping)) {
            shortcuts.at(shortcut + AllowWhileTyping).callback();
        } else if (shortcuts.contains(shortcut)) {
            if (!ImGui::GetIO().WantTextInput)
                shortcuts.at(shortcut).callback();
        }
    }

    void ShortcutManager::process(const std::unique_ptr<View> &currentView, bool ctrl, bool alt, bool shift, bool super, bool focused, u32 keyCode) {
        Shortcut pressedShortcut = getShortcut(ctrl, alt, shift, super, focused, keyCode);
        if (keyCode != 0)
            s_prevShortcut = Shortcut(pressedShortcut.getKeys());

        processShortcut(pressedShortcut, currentView->m_shortcuts);
    }

    void ShortcutManager::processGlobals(bool ctrl, bool alt, bool shift, bool super, u32 keyCode) {
        Shortcut pressedShortcut = getShortcut(ctrl, alt, shift, super, false, keyCode);
        if (keyCode != 0)
            s_prevShortcut = Shortcut(pressedShortcut.getKeys());

        processShortcut(pressedShortcut, s_globalShortcuts);
    }

    void ShortcutManager::clearShortcuts() {
        s_globalShortcuts.clear();
    }

    void ShortcutManager::resumeShortcuts() {
        s_paused = false;
    }

    void ShortcutManager::pauseShortcuts() {
        s_paused = true;
        s_prevShortcut.reset();
    }

    std::optional<Shortcut> ShortcutManager::getPreviousShortcut() {
        return s_prevShortcut;
    }

    std::vector<ShortcutManager::ShortcutEntry> ShortcutManager::getGlobalShortcuts() {
        std::vector<ShortcutManager::ShortcutEntry> result;

        for (auto &[shortcut, entry] : s_globalShortcuts)
            result.push_back(entry);

        return result;
    }

    std::vector<ShortcutManager::ShortcutEntry> ShortcutManager::getViewShortcuts(View *view) {
        std::vector<ShortcutManager::ShortcutEntry> result;

        for (auto &[shortcut, entry] : view->m_shortcuts)
            result.push_back(entry);

        return result;
    }

    static bool updateShortcutImpl(const Shortcut &oldShortcut, const Shortcut &newShortcut, std::map<Shortcut, ShortcutManager::ShortcutEntry> &shortcuts) {
        if (shortcuts.contains(oldShortcut + CurrentView)) {
            if (shortcuts.contains(newShortcut + CurrentView))
                return false;

            shortcuts[newShortcut + CurrentView] = shortcuts[oldShortcut + CurrentView];
            shortcuts[newShortcut + CurrentView].shortcut = newShortcut + CurrentView;
            shortcuts.erase(oldShortcut + CurrentView);
        }

        return true;
    }

    bool ShortcutManager::updateShortcut(const Shortcut &oldShortcut, const Shortcut &newShortcut, View *view) {
        if (oldShortcut == newShortcut)
            return true;

        bool result;
        if (view != nullptr) {
            result = updateShortcutImpl(oldShortcut, newShortcut, view->m_shortcuts);
        } else {
            result = updateShortcutImpl(oldShortcut, newShortcut, s_globalShortcuts);
        }

        if (result) {
            for (auto &[priority, menuItem] : ContentRegistry::Interface::impl::getMenuItems()) {
                if (menuItem.view == view && *menuItem.shortcut == oldShortcut) {
                    *menuItem.shortcut = newShortcut;
                    break;
                }
            }
        }

        return result;
    }

}