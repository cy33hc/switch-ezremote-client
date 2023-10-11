#include <switch.h>
#include <stdio.h>
#include <algorithm>
#include <set>
#include "windows.h"
#include "fs.h"
#include "config.h"
#include "gui.h"
#include "actions.h"
#include "util.h"
#include "lang.h"
#include "ime_dialog.h"

extern "C"
{
#include "inifile.h"
}

static u64 pad_prev;
bool paused = false;
int view_mode;
static float scroll_direction = 0.0f;
static ime_callback_t ime_callback = nullptr;
static ime_callback_t ime_after_update = nullptr;
static ime_callback_t ime_before_update = nullptr;
static ime_callback_t ime_cancelled = nullptr;
static std::vector<std::string> *ime_multi_field;
static char *ime_single_field;
static int ime_field_size;

bool handle_updates = false;
int64_t bytes_transfered;
int64_t bytes_to_download;
std::vector<DirEntry> local_files;
std::vector<DirEntry> remote_files;
std::set<DirEntry> multi_selected_local_files;
std::set<DirEntry> multi_selected_remote_files;
std::vector<DirEntry> local_paste_files;
std::vector<DirEntry> remote_paste_files;
ACTIONS paste_action;
DirEntry selected_local_file;
DirEntry selected_remote_file;
ACTIONS selected_action;
char status_message[1024];
char local_file_to_select[256];
char remote_file_to_select[256];
char local_filter[64];
char remote_filter[64];
char editor_text[1024];
char activity_message[1024];
int selected_browser = 0;
int saved_selected_browser;
bool activity_inprogess = false;
bool stop_activity = false;
bool file_transfering = false;
bool set_focus_to_local = false;
bool set_focus_to_remote = false;

bool dont_prompt_overwrite = false;
bool dont_prompt_overwrite_cb = false;
int confirm_transfer_state = -1;
int overwrite_type = OVERWRITE_PROMPT;

int confirm_state = CONFIRM_NONE;
char confirm_message[256];
ACTIONS action_to_take = ACTION_NONE;
PadState padstate;

namespace Windows
{

    void Init()
    {
        remoteclient = nullptr;

        sprintf(local_file_to_select, "..");
        sprintf(remote_file_to_select, "..");
        sprintf(status_message, "");
        sprintf(local_filter, "");
        sprintf(remote_filter, "");
        dont_prompt_overwrite = false;
        confirm_transfer_state = -1;
        dont_prompt_overwrite_cb = false;
        overwrite_type = OVERWRITE_PROMPT;

        Actions::RefreshLocalFiles(false);
    }

    void HandleWindowInput(u64 pad)
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;

        if ((pad_prev & HidNpadButton_Y) && !(pad & HidNpadButton_Y) && !paused)
        {
            if (selected_browser & LOCAL_BROWSER && strcmp(selected_local_file.name, "..") != 0)
            {
                auto search_item = multi_selected_local_files.find(selected_local_file);
                if (search_item != multi_selected_local_files.end())
                {
                    multi_selected_local_files.erase(search_item);
                }
                else
                {
                    multi_selected_local_files.insert(selected_local_file);
                }
            }
            if (selected_browser & REMOTE_BROWSER && strcmp(selected_remote_file.name, "..") != 0)
            {
                auto search_item = multi_selected_remote_files.find(selected_remote_file);
                if (search_item != multi_selected_remote_files.end())
                {
                    multi_selected_remote_files.erase(search_item);
                }
                else
                {
                    multi_selected_remote_files.insert(selected_remote_file);
                }
            }
        }

        if ((pad_prev & HidNpadButton_R) && !(pad & HidNpadButton_R) && !paused)
        {
            set_focus_to_remote = true;
        }

        if ((pad_prev & HidNpadButton_L) && !(pad & HidNpadButton_L) && !paused)
        {
            set_focus_to_local = true;
        }

        if ((pad_prev & HidNpadButton_Plus) && !(pad & HidNpadButton_Plus) && !paused)
        {
            selected_action = ACTION_DISCONNECT_AND_EXIT;
        }

        pad_prev = pad;
    }

    void SetModalMode(bool modal)
    {
        paused = modal;
    }

    void ConnectionPanel()
    {
        ImGuiStyle *style = &ImGui::GetStyle();
        ImVec4 *colors = style->Colors;
        static char title[64];
        sprintf(title, "ezRemote %s", lang_strings[STR_CONNECTION_SETTINGS]);
        BeginGroupPanel(title, ImVec2(1265, 100));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        char id[256];
        std::string hidden_password = (strlen(remote_settings->password) > 0) ? std::string("*******") : "";

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
        bool is_connected = remoteclient != nullptr && remoteclient->IsConnected();

        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetItemDefaultFocus();
        }
        sprintf(id, "%s###connectbutton", is_connected ? lang_strings[STR_DISCONNECT] : lang_strings[STR_CONNECT]);
        if (ImGui::Button(id, ImVec2(140, 0)))
        {
            selected_action = is_connected ? ACTION_DISCONNECT : ACTION_CONNECT;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", is_connected ? lang_strings[STR_DISCONNECT] : lang_strings[STR_CONNECT]);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();

        if (is_connected)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
        }

        ImGui::SetNextItemWidth(80);
        if (ImGui::BeginCombo("##Site", display_site, ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLargest | ImGuiComboFlags_NoArrowButton))
        {
            static char site_id[64];
            for (int n = 0; n < sites.size(); n++)
            {
                const bool is_selected = strcmp(sites[n].c_str(), last_site) == 0;
                sprintf(site_id, "%s %d", lang_strings[STR_SITE], n + 1);
                if (ImGui::Selectable(site_id, is_selected))
                {
                    sprintf(last_site, "%s", sites[n].c_str());
                    sprintf(display_site, "%s", site_id);
                    remote_settings = &site_settings[sites[n]];
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (is_connected)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 1.0f));
        sprintf(id, "%s##server", remote_settings->server);
        int width = 450;
        if (remote_settings->type == CLIENT_TYPE_NFS)
            width = 700;

        if (ImGui::Button(id, ImVec2(width, 0)))
        {
            ime_single_field = remote_settings->server;
            ResetImeCallbacks();
            ime_field_size = 255;
            ime_callback = SingleValueImeCallback;
            Dialog::initImeDialog(lang_strings[STR_SERVER], remote_settings->server, 255, SwkbdType_Normal, 0, 0);
            gui_mode = GUI_MODE_IME;
        }
        ImGui::SameLine();

        if (remote_settings->type != CLIENT_TYPE_NFS)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
            ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_USERNAME]);
            ImGui::SameLine();
            sprintf(id, "%s##username", remote_settings->username);
            if (ImGui::Button(id, ImVec2(100, 0)))
            {
                ime_single_field = remote_settings->username;
                ResetImeCallbacks();
                ime_field_size = 32;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog(lang_strings[STR_USERNAME], remote_settings->username, 32, SwkbdType_Normal, 0, 0);
                gui_mode = GUI_MODE_IME;
            }
            ImGui::SameLine();

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
            ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_PASSWORD]);
            ImGui::SameLine();
            sprintf(id, "%s##password", hidden_password.c_str());
            if (ImGui::Button(id, ImVec2(60, 0)))
            {
                ime_single_field = remote_settings->password;
                ResetImeCallbacks();
                ime_field_size = 24;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog(lang_strings[STR_PASSWORD], remote_settings->password, 24, SwkbdType_Normal, 0, 0);
                gui_mode = GUI_MODE_IME;
            }
        }
        ImGui::PopStyleVar();

        ImGui::Dummy(ImVec2(0, 10));
        EndGroupPanel();
    }

    void BrowserPanel()
    {
        ImGuiStyle *style = &ImGui::GetStyle();
        ImVec4 *colors = style->Colors;
        selected_browser = 0;

        ImGui::Dummy(ImVec2(0, 5));
        BeginGroupPanel(lang_strings[STR_LOCAL], ImVec2(628, 420));
        ImGui::Dummy(ImVec2(0, 10));

        float posX = ImGui::GetCursorPosX();
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 1.0f));
        ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_DIRECTORY]);
        ImGui::SameLine();
        ImVec2 size = ImGui::CalcTextSize(local_directory);
        ImGui::SetCursorPosX(posX + 110);
        ImGui::PushID("local_directory##local");
        if (ImGui::Button(local_directory, ImVec2(375, 0)))
        {
            ime_single_field = local_directory;
            ResetImeCallbacks();
            ime_field_size = 255;
            ime_after_update = AfterLocalFileChangesCallback;
            ime_callback = SingleValueImeCallback;
            Dialog::initImeDialog(lang_strings[STR_DIRECTORY], local_directory, 256, SwkbdType_Normal, 0, 0);
            gui_mode = GUI_MODE_IME;
        }
        ImGui::PopID();
        ImGui::PopStyleVar();
        if (size.x > 275 && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(local_directory);
            ImGui::EndTooltip();
        }
        ImGui::SameLine();

        ImGui::PushID("refresh##local");
        if (ImGui::Button(lang_strings[STR_REFRESH], ImVec2(110, 25)))
        {
            selected_action = ACTION_REFRESH_LOCAL_FILES;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(lang_strings[STR_REFRESH]);
            ImGui::EndTooltip();
        }
        ImGui::PopID();

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 1.0f));
        ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_FILTER]);
        ImGui::SameLine();
        ImGui::SetCursorPosX(posX + 110);
        ImGui::PushID("local_filter##local");
        if (ImGui::Button(local_filter, ImVec2(375, 0)))
        {
            ime_single_field = local_filter;
            ResetImeCallbacks();
            ime_field_size = 63;
            ime_callback = SingleValueImeCallback;
            Dialog::initImeDialog(lang_strings[STR_FILTER], local_filter, 63, SwkbdType_Normal, 0, 0);
            gui_mode = GUI_MODE_IME;
        }
        ImGui::PopID();
        ImGui::PopStyleVar();
        ImGui::SameLine();

        ImGui::PushID("search##local");
        if (ImGui::Button(lang_strings[STR_SEARCH], ImVec2(110, 25)))
        {
            selected_action = ACTION_APPLY_LOCAL_FILTER;
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(lang_strings[STR_SEARCH]);
            ImGui::EndTooltip();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        ImGui::BeginChild("Local##ChildWindow", ImVec2(609, 420));
        ImGui::Separator();
        ImGui::Columns(2, "Local##Columns", true);
        int i = 0;
        if (set_focus_to_local)
        {
            set_focus_to_local = false;
            ImGui::SetWindowFocus();
        }
        for (int j = 0; j < local_files.size(); j++)
        {
            DirEntry item = local_files[j];
            ImGui::SetColumnWidth(-1, 460);
            ImGui::PushID(i);
            auto search_item = multi_selected_local_files.find(item);
            if (search_item != multi_selected_local_files.end())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            }
            if (ImGui::Selectable(item.name, false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(609, 0)))
            {
                selected_local_file = item;
                if (item.isDir)
                {
                    selected_action = ACTION_CHANGE_LOCAL_DIRECTORY;
                }
            }
            ImGui::PopID();
            if (ImGui::IsItemFocused())
            {
                selected_local_file = item;
            }
            if (ImGui::IsItemHovered())
            {
                if (ImGui::CalcTextSize(item.name).x > 450)
                {
                    ImGui::BeginTooltip();
                    ImGui::Text(item.name);
                    ImGui::EndTooltip();
                }
            }
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                if (strcmp(local_file_to_select, item.name) == 0)
                {
                    SetNavFocusHere();
                    ImGui::SetScrollHereY(0.5f);
                    sprintf(local_file_to_select, "");
                }
                selected_browser |= LOCAL_BROWSER;
            }
            ImGui::NextColumn();
            ImGui::SetColumnWidth(-1, 120);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(item.display_size).x - ImGui::GetScrollX() - ImGui::GetStyle().ItemSpacing.x);
            ImGui::Text(item.display_size);
            if (search_item != multi_selected_local_files.end())
            {
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();
            ImGui::Separator();
            i++;
        }
        ImGui::Columns(1);
        ImGui::EndChild();
        EndGroupPanel();
        ImGui::SameLine();

        BeginGroupPanel(lang_strings[STR_REMOTE], ImVec2(628, 420));
        ImGui::Dummy(ImVec2(0, 10));
        posX = ImGui::GetCursorPosX();
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 1.0f));
        ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_DIRECTORY]);
        ImGui::SameLine();
        size = ImGui::CalcTextSize(remote_directory);
        ImGui::SetCursorPosX(posX + 110);
        ImGui::PushID("remote_directory##remote");
        if (ImGui::Button(remote_directory, ImVec2(375, 0)))
        {
            ime_single_field = remote_directory;
            ResetImeCallbacks();
            ime_field_size = 255;
            ime_after_update = AfterRemoteFileChangesCallback;
            ime_callback = SingleValueImeCallback;
            Dialog::initImeDialog(lang_strings[STR_DIRECTORY], remote_directory, 256, SwkbdType_Normal, 0, 0);
            gui_mode = GUI_MODE_IME;
        }
        ImGui::PopID();
        ImGui::PopStyleVar();
        if (size.x > 275 && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(remote_directory);
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        ImGui::PushID("refresh##remote");
        if (ImGui::Button(lang_strings[STR_REFRESH], ImVec2(110, 25)))
        {
            selected_action = ACTION_REFRESH_REMOTE_FILES;
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(lang_strings[STR_REFRESH]);
            ImGui::EndTooltip();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 1.0f));
        ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_FILTER]);
        ImGui::SameLine();
        ImGui::SetCursorPosX(posX + 110);
        ImGui::PushID("remote_filter##remote");
        if (ImGui::Button(remote_filter, ImVec2(375, 0)))
        {
            ime_single_field = remote_filter;
            ResetImeCallbacks();
            ime_field_size = 63;
            ime_callback = SingleValueImeCallback;
            Dialog::initImeDialog(lang_strings[STR_FILTER], remote_filter, 63, SwkbdType_Normal, 0, 0);
            gui_mode = GUI_MODE_IME;
        };
        ImGui::PopID();
        ImGui::PopStyleVar();
        ImGui::SameLine();

        ImGui::PushID("search##remote");
        if (ImGui::Button(lang_strings[STR_SEARCH], ImVec2(110, 25)))
        {
            selected_action = ACTION_APPLY_REMOTE_FILTER;
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(lang_strings[STR_SEARCH]);
            ImGui::EndTooltip();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::BeginChild(ImGui::GetID("Remote##ChildWindow"), ImVec2(609, 420));
        if (set_focus_to_remote)
        {
            set_focus_to_remote = false;
            ImGui::SetWindowFocus();
        }
        ImGui::Separator();
        ImGui::Columns(2, "Remote##Columns", true);
        i = 99999;
        for (int j = 0; j < remote_files.size(); j++)
        {
            DirEntry item = remote_files[j];

            ImGui::SetColumnWidth(-1, 460);
            auto search_item = multi_selected_remote_files.find(item);
            if (search_item != multi_selected_remote_files.end())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            }
            ImGui::PushID(i);
            if (ImGui::Selectable(item.name, false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(609, 0)))
            {
                selected_remote_file = item;
                if (item.isDir)
                {
                    selected_action = ACTION_CHANGE_REMOTE_DIRECTORY;
                }
            }
            if (ImGui::IsItemHovered())
            {
                if (ImGui::CalcTextSize(item.name).x > 450)
                {
                    ImGui::BeginTooltip();
                    ImGui::Text(item.name);
                    ImGui::EndTooltip();
                }
            }
            ImGui::PopID();
            if (ImGui::IsItemFocused())
            {
                selected_remote_file = item;
            }
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                if (strcmp(remote_file_to_select, item.name) == 0)
                {
                    SetNavFocusHere();
                    ImGui::SetScrollHereY(0.5f);
                    sprintf(remote_file_to_select, "");
                }
                selected_browser |= REMOTE_BROWSER;
            }
            ImGui::NextColumn();
            ImGui::SetColumnWidth(-1, 120);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(item.display_size).x - ImGui::GetScrollX() - ImGui::GetStyle().ItemSpacing.x);
            ImGui::Text(item.display_size);
            if (search_item != multi_selected_remote_files.end())
            {
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();
            ImGui::Separator();
            i++;
        }
        ImGui::Columns(1);
        ImGui::EndChild();
        EndGroupPanel();
    }

    void StatusPanel()
    {
        ImGui::Dummy(ImVec2(0, 5));
        BeginGroupPanel(lang_strings[STR_MESSAGES], ImVec2(1265, 100));
        ImVec2 pos = ImGui::GetCursorPos();
        ImGui::Dummy(ImVec2(925, 30));
        ImGui::SetCursorPos(pos);
        ImGui::PushTextWrapPos(925);
        if (strncmp(status_message, "4", 1) == 0 || strncmp(status_message, "3", 1) == 0)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), status_message);
        }
        else
        {
            ImGui::Text(status_message);
        }
        ImGui::PopTextWrapPos();
        ImGui::SameLine();
        EndGroupPanel();
    }

    int getSelectableFlag(uint32_t remote_action)
    {
        int flag = ImGuiSelectableFlags_Disabled;
        bool local_browser_selected = saved_selected_browser & LOCAL_BROWSER;
        bool remote_browser_selected = saved_selected_browser & REMOTE_BROWSER;

        if ((local_browser_selected && selected_local_file.selectable) ||
            (remote_browser_selected && selected_remote_file.selectable &&
             remoteclient != nullptr && (remoteclient->SupportedActions() & remote_action)))
        {
            flag = ImGuiSelectableFlags_None;
        }
        return flag;
    }

    void ShowActionsDialog()
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGuiStyle *style = &ImGui::GetStyle();
        ImVec4 *colors = style->Colors;
        int flags;

        if (ImGui::IsKeyDown(ImGuiKey_GamepadFaceUp) && !paused)
        {
            if (!paused)
                saved_selected_browser = selected_browser;

            if (saved_selected_browser > 0)
            {
                SetModalMode(true);
                ImGui::OpenPopup(lang_strings[STR_ACTIONS]);
            }
        }

        bool local_browser_selected = saved_selected_browser & LOCAL_BROWSER;
        bool remote_browser_selected = saved_selected_browser & REMOTE_BROWSER;
        if (local_browser_selected)
        {
            ImGui::SetNextWindowPos(ImVec2(210, 250));
        }
        else if (remote_browser_selected)
        {
            ImGui::SetNextWindowPos(ImVec2(830, 250));
        }
        ImGui::SetNextWindowSizeConstraints(ImVec2(230, 150), ImVec2(230, 300), NULL, NULL);
        if (ImGui::BeginPopupModal(lang_strings[STR_ACTIONS], NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushID("Select All##settings");
            if (ImGui::Selectable(lang_strings[STR_SELECT_ALL], false, ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                SetModalMode(false);
                if (local_browser_selected)
                    selected_action = ACTION_LOCAL_SELECT_ALL;
                else if (remote_browser_selected)
                    selected_action = ACTION_REMOTE_SELECT_ALL;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            ImGui::Separator();

            ImGui::PushID("Clear All##settings");
            if (ImGui::Selectable(lang_strings[STR_CLEAR_ALL], false, ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                SetModalMode(false);
                if (local_browser_selected)
                    selected_action = ACTION_LOCAL_CLEAR_ALL;
                else if (remote_browser_selected)
                    selected_action = ACTION_REMOTE_CLEAR_ALL;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            ImGui::Separator();

            ImGui::PushID("Delete##settings");
            if (ImGui::Selectable(lang_strings[STR_DELETE], false, getSelectableFlag(REMOTE_ACTION_DELETE) | ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                confirm_state = CONFIRM_WAIT;
                sprintf(confirm_message, lang_strings[STR_DEL_CONFIRM_MSG]);
                if (local_browser_selected)
                    action_to_take = ACTION_DELETE_LOCAL;
                else if (remote_browser_selected)
                    action_to_take = ACTION_DELETE_REMOTE;
            }
            ImGui::PopID();
            ImGui::Separator();

            flags = getSelectableFlag(REMOTE_ACTION_RENAME);
            if ((local_browser_selected && multi_selected_local_files.size() > 1) ||
                (remote_browser_selected && multi_selected_remote_files.size() > 1))
                flags = ImGuiSelectableFlags_None;
            ImGui::PushID("Rename##settings");
            if (ImGui::Selectable(lang_strings[STR_RENAME], false, flags | ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                if (local_browser_selected)
                    selected_action = ACTION_RENAME_LOCAL;
                else if (remote_browser_selected)
                    selected_action = ACTION_RENAME_REMOTE;
                SetModalMode(false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            ImGui::Separator();

            flags = ImGuiSelectableFlags_None;
            if (remote_browser_selected && remoteclient != nullptr && !(remoteclient->SupportedActions() & REMOTE_ACTION_NEW_FOLDER))
            {
                flags = ImGuiSelectableFlags_Disabled;
            }
            ImGui::PushID("New Folder##settings");
            if (ImGui::Selectable(lang_strings[STR_NEW_FOLDER], false, flags | ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                if (local_browser_selected)
                    selected_action = ACTION_NEW_LOCAL_FOLDER;
                else if (remote_browser_selected)
                    selected_action = ACTION_NEW_REMOTE_FOLDER;
                SetModalMode(false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            ImGui::Separator();

            flags = ImGuiSelectableFlags_Disabled;
            if (local_browser_selected)
            {
                flags = getSelectableFlag(REMOTE_ACTION_UPLOAD);
                if (local_browser_selected && remoteclient != nullptr && !(remoteclient->SupportedActions() & REMOTE_ACTION_UPLOAD))
                {
                    flags = ImGuiSelectableFlags_Disabled;
                }
                ImGui::PushID("Upload##settings");
                if (ImGui::Selectable(lang_strings[STR_UPLOAD], false, flags | ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
                {
                    SetModalMode(false);
                    selected_action = ACTION_UPLOAD;
                    file_transfering = true;
                    confirm_transfer_state = 0;
                    dont_prompt_overwrite_cb = dont_prompt_overwrite;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
                ImGui::Separator();
            }

            if (remote_browser_selected)
            {
                if (multi_selected_remote_files.size() > 0)
                {
                    flags = ImGuiSelectableFlags_None;
                }
                ImGui::PushID("Download##settings");
                if (ImGui::Selectable(lang_strings[STR_DOWNLOAD], false, getSelectableFlag(REMOTE_ACTION_DOWNLOAD) | ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
                {
                    SetModalMode(false);
                    selected_action = ACTION_DOWNLOAD;
                    file_transfering = true;
                    confirm_transfer_state = 0;
                    dont_prompt_overwrite_cb = dont_prompt_overwrite;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
                ImGui::Separator();
            }

            flags = ImGuiSelectableFlags_Disabled;
            if (local_browser_selected || remote_browser_selected)
                flags = ImGuiSelectableFlags_None;
            ImGui::PushID("Properties##settings");
            if (ImGui::Selectable(lang_strings[STR_PROPERTIES], false, flags | ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                if (local_browser_selected)
                    selected_action = ACTION_SHOW_LOCAL_PROPERTIES;
                else if (remote_browser_selected)
                    selected_action = ACTION_SHOW_REMOTE_PROPERTIES;
                SetModalMode(false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();

            ImGui::Separator();

            ImGui::PushID("Cancel##settings");
            if (ImGui::Selectable(lang_strings[STR_CANCEL], false, ImGuiSelectableFlags_DontClosePopups, ImVec2(220, 0)))
            {
                SetModalMode(false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetItemDefaultFocus();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false))
            {
                SetModalMode(false);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (confirm_state == CONFIRM_WAIT)
        {
            ImGui::OpenPopup(lang_strings[STR_CONFIRM]);
            ImGui::SetNextWindowPos(ImVec2(400, 250));
            ImGui::SetNextWindowSizeConstraints(ImVec2(500, 100), ImVec2(500, 200), NULL, NULL);
            if (ImGui::BeginPopupModal(lang_strings[STR_CONFIRM], NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 485);
                ImGui::Text(confirm_message);
                ImGui::PopTextWrapPos();
                ImGui::NewLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 165);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                if (ImGui::Button(lang_strings[STR_NO], ImVec2(60, 0)))
                {
                    confirm_state = CONFIRM_NO;
                    selected_action = ACTION_NONE;
                    SetModalMode(false);
                    ImGui::CloseCurrentPopup();
                };
                ImGui::SameLine();
                if (ImGui::Button(lang_strings[STR_YES], ImVec2(60, 0)))
                {
                    confirm_state = CONFIRM_YES;
                    selected_action = action_to_take;
                    SetModalMode(false);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (confirm_transfer_state == 0)
        {
            ImGui::OpenPopup(lang_strings[STR_OVERWRITE_OPTIONS]);
            ImGui::SetNextWindowPos(ImVec2(400, 220));
            ImGui::SetNextWindowSizeConstraints(ImVec2(480, 100), ImVec2(480, 400), NULL, NULL);
            if (ImGui::BeginPopupModal(lang_strings[STR_OVERWRITE_OPTIONS], NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::RadioButton(lang_strings[STR_DONT_OVERWRITE], &overwrite_type, 0);
                ImGui::RadioButton(lang_strings[STR_ASK_FOR_CONFIRM], &overwrite_type, 1);
                ImGui::RadioButton(lang_strings[STR_DONT_ASK_CONFIRM], &overwrite_type, 2);
                ImGui::Separator();
                ImGui::Checkbox("##AlwaysUseOption", &dont_prompt_overwrite_cb);
                ImGui::SameLine();
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 465);
                ImGui::Text(lang_strings[STR_ALLWAYS_USE_OPTION]);
                ImGui::PopTextWrapPos();
                ImGui::Separator();

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 140);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                if (ImGui::Button(lang_strings[STR_CANCEL], ImVec2(100, 0)))
                {
                    confirm_transfer_state = 2;
                    dont_prompt_overwrite_cb = dont_prompt_overwrite;
                    selected_action = ACTION_NONE;
                    ImGui::CloseCurrentPopup();
                };
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::SameLine();
                if (ImGui::Button(lang_strings[STR_CONTINUE], ImVec2(100, 0)))
                {
                    confirm_transfer_state = 1;
                    dont_prompt_overwrite = dont_prompt_overwrite_cb;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    void ShowPropertiesDialog(DirEntry item)
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGuiStyle *style = &ImGui::GetStyle();
        ImVec4 *colors = style->Colors;
        SetModalMode(true);
        ImGui::OpenPopup(lang_strings[STR_PROPERTIES]);

        ImGui::SetNextWindowPos(ImVec2(380, 250));
        ImGui::SetNextWindowSizeConstraints(ImVec2(500, 80), ImVec2(500, 250), NULL, NULL);
        if (ImGui::BeginPopupModal(lang_strings[STR_PROPERTIES], NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_TYPE]);
            ImGui::SameLine();
            ImGui::SetCursorPosX(105);
            ImGui::Text(item.isDir ? lang_strings[STR_FOLDER] : lang_strings[STR_FILE]);
            ImGui::Separator();

            ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_NAME]);
            ImGui::SameLine();
            ImGui::SetCursorPosX(105);
            ImGui::TextWrapped(item.name);
            ImGui::Separator();

            ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_SIZE]);
            ImGui::SameLine();
            ImGui::SetCursorPosX(105);
            ImGui::Text("%lld   (%s)", item.file_size, item.display_size);
            ImGui::Separator();

            ImGui::TextColored(colors[ImGuiCol_ButtonHovered], "%s:", lang_strings[STR_DATE]);
            ImGui::SameLine();
            ImGui::SetCursorPosX(105);
            ImGui::Text("%02d/%02d/%d %02d:%02d:%02d", item.modified.day, item.modified.month, item.modified.year,
                        item.modified.hours, item.modified.minutes, item.modified.seconds);
            ImGui::Separator();

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 190);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            if (ImGui::Button(lang_strings[STR_CLOSE], ImVec2(100, 0)))
            {
                SetModalMode(false);
                selected_action = ACTION_NONE;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void ShowProgressDialog()
    {
        if (activity_inprogess)
        {
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            ImGuiStyle *style = &ImGui::GetStyle();
            ImVec4 *colors = style->Colors;

            SetModalMode(true);
            ImGui::OpenPopup(lang_strings[STR_PROGRESS]);

            ImGui::SetNextWindowPos(ImVec2(380, 250));
            ImGui::SetNextWindowSizeConstraints(ImVec2(520, 80), ImVec2(520, 200), NULL, NULL);
            if (ImGui::BeginPopupModal(lang_strings[STR_PROGRESS], NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImVec2 cur_pos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(cur_pos);
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 490);
                ImGui::Text("%s", activity_message);
                ImGui::PopTextWrapPos();
                ImGui::SetCursorPosY(cur_pos.y + 60);

                if (file_transfering)
                {
                    static float progress = 0.0f;
                    progress = (float)bytes_transfered / (float)bytes_to_download;
                    ImGui::ProgressBar(progress, ImVec2(505, 0));
                }

                ImGui::Separator();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 225);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                if (ImGui::Button(lang_strings[STR_CANCEL]))
                {
                    stop_activity = true;
                    SetModalMode(false);
                }
                if (stop_activity)
                {
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 490);
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), lang_strings[STR_CANCEL_ACTION_MSG]);
                    ImGui::PopTextWrapPos();
                }
                ImGui::EndPopup();
            }
        }
    }

    void MainWindow()
    {
        Windows::SetupWindow();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        if (ImGui::Begin("ezRemote Client", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar))
        {
            ConnectionPanel();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            BrowserPanel();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            StatusPanel();
            ShowProgressDialog();
            ShowActionsDialog();
        }
        ImGui::End();
    }

    void ExecuteActions()
    {
        switch (selected_action)
        {
        case ACTION_CHANGE_LOCAL_DIRECTORY:
            Actions::HandleChangeLocalDirectory(selected_local_file);
            break;
        case ACTION_CHANGE_REMOTE_DIRECTORY:
            Actions::HandleChangeRemoteDirectory(selected_remote_file);
            break;
        case ACTION_REFRESH_LOCAL_FILES:
            Actions::HandleRefreshLocalFiles();
            break;
        case ACTION_REFRESH_REMOTE_FILES:
            Actions::HandleRefreshRemoteFiles();
            break;
        case ACTION_APPLY_LOCAL_FILTER:
            Actions::RefreshLocalFiles(true);
            selected_action = ACTION_NONE;
            break;
        case ACTION_APPLY_REMOTE_FILTER:
            Actions::RefreshRemoteFiles(true);
            selected_action = ACTION_NONE;
            break;
        case ACTION_NEW_LOCAL_FOLDER:
        case ACTION_NEW_REMOTE_FOLDER:
            if (gui_mode != GUI_MODE_IME)
            {
                sprintf(editor_text, "");
                ime_single_field = editor_text;
                ResetImeCallbacks();
                ime_field_size = 128;
                ime_after_update = AfterFolderNameCallback;
                ime_cancelled = CancelActionCallBack;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog(lang_strings[STR_NEW_FOLDER], editor_text, 128, SwkbdType_Normal, 0, 0);
                gui_mode = GUI_MODE_IME;
            }
            break;
        case ACTION_DELETE_LOCAL:
            activity_inprogess = true;
            sprintf(activity_message, "%s", "");
            stop_activity = false;
            selected_action = ACTION_NONE;
            Actions::DeleteSelectedLocalFiles();
            break;
        case ACTION_DELETE_REMOTE:
            activity_inprogess = true;
            stop_activity = false;
            selected_action = ACTION_NONE;
            Actions::DeleteSelectedRemotesFiles();
            break;
        case ACTION_UPLOAD:
            sprintf(status_message, "%s", "");
            if (dont_prompt_overwrite || (!dont_prompt_overwrite && confirm_transfer_state == 1))
            {
                activity_inprogess = true;
                sprintf(activity_message, "%s", "");
                stop_activity = false;
                Actions::UploadFiles();
                confirm_transfer_state = -1;
                selected_action = ACTION_NONE;
            }
            break;
        case ACTION_DOWNLOAD:
            sprintf(status_message, "%s", "");
            if (dont_prompt_overwrite || (!dont_prompt_overwrite && confirm_transfer_state == 1))
            {
                activity_inprogess = true;
                sprintf(activity_message, "%s", "");
                stop_activity = false;
                Actions::DownloadFiles();
                confirm_transfer_state = -1;
                selected_action = ACTION_NONE;
            }
            break;
        case ACTION_RENAME_LOCAL:
            if (gui_mode != GUI_MODE_IME)
            {
                if (multi_selected_local_files.size() > 0)
                    sprintf(editor_text, "%s", multi_selected_local_files.begin()->name);
                else
                    sprintf(editor_text, "%s", selected_local_file.name);
                ime_single_field = editor_text;
                ResetImeCallbacks();
                ime_field_size = 128;
                ime_after_update = AfterFolderNameCallback;
                ime_cancelled = CancelActionCallBack;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog(lang_strings[STR_RENAME], editor_text, 128, SwkbdType_Normal, 0, 0);
                gui_mode = GUI_MODE_IME;
            }
            break;
        case ACTION_RENAME_REMOTE:
            if (gui_mode != GUI_MODE_IME)
            {
                if (multi_selected_remote_files.size() > 0)
                    sprintf(editor_text, "%s", multi_selected_remote_files.begin()->name);
                else
                    sprintf(editor_text, "%s", selected_remote_file.name);
                ime_single_field = editor_text;
                ResetImeCallbacks();
                ime_field_size = 128;
                ime_after_update = AfterFolderNameCallback;
                ime_cancelled = CancelActionCallBack;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog(lang_strings[STR_RENAME], editor_text, 128, SwkbdType_Normal, 0, 0);
                gui_mode = GUI_MODE_IME;
            }
            break;
        case ACTION_SHOW_LOCAL_PROPERTIES:
            ShowPropertiesDialog(selected_local_file);
            break;
        case ACTION_SHOW_REMOTE_PROPERTIES:
            ShowPropertiesDialog(selected_remote_file);
            break;
        case ACTION_LOCAL_SELECT_ALL:
            Actions::SelectAllLocalFiles();
            selected_action = ACTION_NONE;
            break;
        case ACTION_REMOTE_SELECT_ALL:
            Actions::SelectAllRemoteFiles();
            selected_action = ACTION_NONE;
            break;
        case ACTION_LOCAL_CLEAR_ALL:
            multi_selected_local_files.clear();
            selected_action = ACTION_NONE;
            break;
        case ACTION_REMOTE_CLEAR_ALL:
            multi_selected_remote_files.clear();
            selected_action = ACTION_NONE;
            break;
        case ACTION_CONNECT:
            sprintf(status_message, "%s", "");
            Actions::Connect();
            break;
        case ACTION_DISCONNECT:
            sprintf(status_message, "%s", "");
            Actions::Disconnect();
            break;
        case ACTION_DISCONNECT_AND_EXIT:
            Actions::Disconnect();
            done = true;
            break;
        default:
            break;
        }
    }

    void ResetImeCallbacks()
    {
        ime_callback = nullptr;
        ime_after_update = nullptr;
        ime_before_update = nullptr;
        ime_cancelled = nullptr;
        ime_field_size = 1;
    }

    void HandleImeInput()
    {
        if (Dialog::isImeDialogRunning())
        {
            int ime_result = Dialog::updateImeDialog();
            if (ime_result == IME_DIALOG_RESULT_FINISHED || ime_result == IME_DIALOG_RESULT_CANCELED)
            {
                if (ime_result == IME_DIALOG_RESULT_FINISHED)
                {
                    if (ime_before_update != nullptr)
                    {
                        ime_before_update(ime_result);
                    }

                    if (ime_callback != nullptr)
                    {
                        ime_callback(ime_result);
                    }

                    if (ime_after_update != nullptr)
                    {
                        ime_after_update(ime_result);
                    }
                }
                else if (ime_cancelled != nullptr)
                {
                    ime_cancelled(ime_result);
                }

                ResetImeCallbacks();
                gui_mode = GUI_MODE_BROWSER;
            }
        }
    }

    void SingleValueImeCallback(int ime_result)
    {
        if (ime_result == IME_DIALOG_RESULT_FINISHED)
        {
            char *new_value = (char *)Dialog::getImeDialogInputText();
            snprintf(ime_single_field, ime_field_size, "%s", new_value);
        }
    }

    void MultiValueImeCallback(int ime_result)
    {
        if (ime_result == IME_DIALOG_RESULT_FINISHED)
        {
            char *new_value = (char *)Dialog::getImeDialogInputText();
            char *initial_value = (char *)Dialog::getImeDialogInitialText();
            if (strlen(initial_value) == 0)
            {
                ime_multi_field->push_back(std::string(new_value));
            }
            else
            {
                for (int i = 0; i < ime_multi_field->size(); i++)
                {
                    if (strcmp((*ime_multi_field)[i].c_str(), initial_value) == 0)
                    {
                        (*ime_multi_field)[i] = std::string(new_value);
                    }
                }
            }
        }
    }

    void NullAfterValueChangeCallback(int ime_result) {}

    void AfterLocalFileChangesCallback(int ime_result)
    {
        selected_action = ACTION_REFRESH_LOCAL_FILES;
    }

    void AfterRemoteFileChangesCallback(int ime_result)
    {
        selected_action = ACTION_REFRESH_REMOTE_FILES;
    }

    void AfterFolderNameCallback(int ime_result)
    {
        if (selected_action == ACTION_NEW_LOCAL_FOLDER)
        {
            Actions::CreateNewLocalFolder(editor_text);
        }
        else if (selected_action == ACTION_NEW_REMOTE_FOLDER)
        {
            Actions::CreateNewRemoteFolder(editor_text);
        }
        else if (selected_action == ACTION_RENAME_LOCAL)
        {
            Actions::RenameLocalFolder(multi_selected_local_files.begin()->path, editor_text);
        }
        else if (selected_action == ACTION_RENAME_REMOTE)
        {
            Actions::RenameRemoteFolder(multi_selected_remote_files.begin()->path, editor_text);
        }
        selected_action = ACTION_NONE;
    }

    void CancelActionCallBack(int ime_result)
    {
        selected_action = ACTION_NONE;
    }
}