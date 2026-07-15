#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================
//  Disclaimer
// ============================================================
//  This tool is free for personal Windows cleanup use.
//  Close important programs and back up data before use.
//  This software contains no malware, but absolute safety
//  in all environments is not guaranteed.
//  By using this tool you agree: the developer is not liable
//  for any direct or indirect damages (data loss, etc.).
// ============================================================

struct Lang {
    const char* title;
    const char* disclaimer;
    const char* disc_line1;
    const char* disc_line2;
    const char* disc_line3;
    const char* disc_line4;
    const char* disc_line5;
    const char* scope_line1;
    const char* scope_line2;
    const char* mode_admin;
    const char* mode_user;
    const char* mode_user_hint;
    const char* lang_choose;
    const char* lang_invalid;
    const char* prompt_yes;
    const char* scanning;
    const char* skipped_admin;
    const char* total_cleanable;
    const char* level_label;
    const char* skipped_count;
    const char* summary_title;
    const char* summary_items;
    const char* summary_size;
    const char* summary_choose;
    const char* summary_all;
    const char* summary_select;
    const char* summary_no;
    const char* prompt_mode;
    const char* prompt_select;
    const char* select_list_title;
    const char* select_confirm_title;
    const char* select_will_clean;
    const char* select_confirm;
    const char* no_valid_sel;
    const char* clean_title;
    const char* cleaning;
    const char* freed;
    const char* done_title;
    const char* done_freed;
    const char* done_failed;
    const char* cancelled;
    const char* press_enter;
    const char* no_junk;
    const char* level_light;
    const char* level_medium;
    const char* level_moderate;
    const char* level_heavy;
    const char* level_severe;
    const char* name_user_temp;
    const char* name_chrome_cache;
    const char* name_chrome_code;
    const char* name_edge_cache;
    const char* name_edge_code;
    const char* name_firefox_cache;
    const char* name_thumb_cache;
    const char* name_win_temp;
    const char* name_recycle;
    const char* name_prefetch;
    const char* name_win_update;
    const char* name_wer;
    const char* name_win_logs;
    const char* path_not_found;
};

const Lang ZH = {
    "C盘垃圾清理工具  v1.4.0",
    "免 责 声 明",
    "本工具免费提供，仅供个人清理系统垃圾使用。",
    "使用前请关闭重要程序，并提前备份数据。",
    "本软件无恶意代码，但无法保证在所有环境绝对安全。",
    "使用即视为同意：开发者对任何直接或间接损失",
    "（包括数据丢失、系统损坏等）不承担责任。",
    "清理范围：Temp / 回收站 / 浏览器缓存 / Prefetch",
    "         更新缓存 / 错误报告 / 日志 / 缩略图",
    "[管理员模式] 完整清理 - 全部 13 项可用",
    "[普通用户模式] 仅清理用户目录垃圾 - 7 项",
    "以管理员身份运行可解锁全部清理能力",
    "请选择语言 / Choose language: [1] 中文  [2] English",
    "无效选择，请输入 1 或 2",
    "输入 YES 开始扫描，其他任意键退出: ",
    "正在扫描垃圾文件...",
    "跳过（需管理员权限）",
    "总计可清理",
    "垃圾量评级",
    "跳过项（需管理员权限）",
    "扫描报告汇总",
    "可清理项目",
    "总大小",
    "请选择清理方式：",
    "[A] 全部清理",
    "[S] 自定义选择",
    "[N] 不清理，退出",
    "请输入 A / S / N: ",
    "请输入要清理的编号（如 1,3-5）: ",
    "可选项目列表（逗号分隔，支持范围如 3-5）:",
    "已选择以下项目:",
    "本次将清理",
    "确认清理？输入 Y 执行，其他任意键取消: ",
    "没有有效选择，已取消。",
    "开始清理垃圾文件...",
    "清理",
    "释放",
    "清理完成 - 汇总报告",
    "已清理空间",
    "失败项",
    "已取消清理。按回车退出...",
    "按回车退出...",
    "没有发现可清理的垃圾文件。",
    "轻度   (< 500 MB)",
    "中等   (500 MB ~ 2 GB)",
    "较多   (2 GB ~ 5 GB)",
    "大量   (5 GB ~ 10 GB)",
    "严重   (> 10 GB)",
    "用户 Temp",
    "Chrome 缓存",
    "Chrome Code Cache",
    "Edge 缓存",
    "Edge Code Cache",
    "Firefox 缓存",
    "缩略图缓存",
    "Windows Temp",
    "回收站",
    "预读取文件 (Prefetch)",
    "Windows 更新缓存",
    "Windows 错误报告",
    "Windows 日志",
    "路径不存在，跳过。"
};

const Lang EN = {
    "C Drive Junk Cleaner  v1.4.0",
    "DISCLAIMER",
    "This tool is free for personal Windows cleanup use.",
    "Close important programs and back up data before use.",
    "This software contains no malware, but absolute safety",
    "in all environments is not guaranteed.",
    "By using this tool you agree: the developer is not liable",
    "Scope: Temp / RecycleBin / Browser Cache / Prefetch",
    "       Update Cache / Error Reports / Logs / Thumbnails",
    "[Admin Mode] Full cleanup - all 13 targets available",
    "[User Mode] User-directory only - 7 targets",
    "Run as Administrator for full cleanup capability",
    "Please choose language: [1] Chinese  [2] English",
    "Invalid choice, enter 1 or 2",
    "Type YES to start scanning, anything else to exit: ",
    "Scanning junk files...",
    "Skipped (requires admin)",
    "Total cleanable",
    "Junk level",
    "Skipped (requires admin)",
    "Scan Report",
    "Cleanable items",
    "Total size",
    "Choose cleanup mode:",
    "[A] Clean All",
    "[S] Select items",
    "[N] Cancel, exit",
    "Enter A / S / N: ",
    "Enter numbers to clean (e.g. 1,3-5): ",
    "Cleanable items (comma-separated, range like 3-5):",
    "Selected items:",
    "Will clean",
    "Confirm? Enter Y to proceed, anything else to cancel: ",
    "No valid selection, cancelled.",
    "Cleaning junk files...",
    "Cleaning",
    "Freed",
    "Cleanup Complete - Summary",
    "Space Freed",
    "Failed items",
    "Cleanup cancelled. Press Enter to exit...",
    "Press Enter to exit...",
    "No junk files found to clean.",
    "Light  (< 500 MB)",
    "Medium (500 MB ~ 2 GB)",
    "Moderate (2 GB ~ 5 GB)",
    "Heavy  (5 GB ~ 10 GB)",
    "Severe (> 10 GB)",
    "User Temp",
    "Chrome Cache",
    "Chrome Code Cache",
    "Edge Cache",
    "Edge Code Cache",
    "Firefox Cache",
    "Thumbnail Cache",
    "Windows Temp",
    "Recycle Bin",
    "Prefetch",
    "Windows Update Cache",
    "Windows Error Reports",
    "Windows Logs",
    "Path not found, skipped."
};

const Lang* g = &ZH;

// ---------- Utilities ----------
struct CleanTarget {
    std::string  name;
    std::wstring path;
    bool         isFolder;
    bool         requiresAdmin;
};

ULONGLONG g_totalDeleted = 0;
ULONGLONG g_totalFailed  = 0;
int       g_totalSkipped = 0;

std::string FormatSize(ULONGLONG bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int idx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %s", val, units[idx]);
    return buf;
}

const char* ClassifySize(ULONGLONG bytes) {
    if (bytes < 500ULL * 1024 * 1024)       return g->level_light;
    if (bytes < 2ULL * 1024 * 1024 * 1024)  return g->level_medium;
    if (bytes < 5ULL * 1024 * 1024 * 1024)  return g->level_moderate;
    if (bytes < 10ULL * 1024 * 1024 * 1024) return g->level_heavy;
    return                                      g->level_severe;
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token, TokenElevation, &elevation, size, &size))
            isAdmin = elevation.TokenIsElevated;
        CloseHandle(token);
    }
    return isAdmin != FALSE;
}

ULONGLONG ScanFolder(const std::wstring& folder) {
    ULONGLONG total = 0;
    std::error_code ec;
    fs::path fp(folder);
    if (!fs::exists(fp, ec)) return 0;
    for (auto& entry : fs::recursive_directory_iterator(fp, ec)) {
        if (ec) { ec.clear(); continue; }
        if (fs::is_regular_file(entry, ec)) {
            ULONGLONG sz = fs::file_size(entry, ec);
            if (!ec) total += sz;
        }
    }
    return total;
}

ULONGLONG DeleteFolder(const std::wstring& folder) {
    ULONGLONG freed = 0;
    std::error_code ec;
    fs::path fp(folder);
    if (!fs::exists(fp, ec)) return 0;
    for (auto& entry : fs::recursive_directory_iterator(fp, ec)) {
        if (ec) { ec.clear(); continue; }
        if (fs::is_regular_file(entry, ec)) {
            ULONGLONG sz = fs::file_size(entry, ec);
            if (!ec) freed += sz;
            fs::remove(entry, ec);
            if (ec) g_totalFailed++;
        }
    }
    for (auto& entry : fs::recursive_directory_iterator(fp, ec)) {
        if (ec) { ec.clear(); continue; }
        fs::remove(entry, ec);
    }
    fs::remove(fp, ec);
    return freed;
}

struct ScanResult {
    std::string name;
    ULONGLONG   size;
    bool        skipped;
    int         index;
};

// ---------- Language selection ----------
void ChooseLanguage() {
    system("cls");
    std::cout << R"(
+------------------------------------------------------+
|            C Drive Junk Cleaner  v1.4.0               |
|               C盘垃圾清理工具  v1.4.0                 |
+------------------------------------------------------+
|                                                      |
|     [1]  中文  (Chinese)                             |
|     [2]  English                                     |
|                                                      |
+------------------------------------------------------+
)";
    std::cout << "\n  " << g->lang_choose;
    std::string input;
    std::getline(std::cin, input);
    if (input == "2")      g = &EN;
    else if (input == "1") g = &ZH;
}

// ---------- Disclaimer ----------
bool ShowDisclaimer(bool isAdmin) {
    system("cls");
    std::cout << "\n";
    std::cout << "+------------------------------------------------------+\n";
    std::cout << "|            " << g->title << "                    |\n";
    std::cout << "+------------------------------------------------------+\n";
    std::cout << "|                 ** " << g->disclaimer << " **              |\n";
    std::cout << "|                                                      |\n";
    std::cout << "|  " << g->disc_line1 << "  |\n";
    std::cout << "|  " << g->disc_line2 << "  |\n";
    std::cout << "|  " << g->disc_line3 << "  |\n";
    std::cout << "|  " << g->disc_line4 << "  |\n";
    std::cout << "|  " << g->disc_line5 << "  |\n";
    std::cout << "|                                                      |\n";
    std::cout << "|  " << g->scope_line1 << "  |\n";
    std::cout << "|  " << g->scope_line2 << "  |\n";
    std::cout << "+------------------------------------------------------+\n";

    if (isAdmin)
        std::cout << "\n  >> " << g->mode_admin << "\n";
    else {
        std::cout << "\n  >> " << g->mode_user << "\n";
        std::cout << "  >> " << g->mode_user_hint << "\n";
    }

    std::cout << "\n  " << g->prompt_yes;
    std::string input;
    std::getline(std::cin, input);
    return (input == "YES" || input == "yes" || input == "Yes");
}

// ---------- Scan ----------
std::vector<ScanResult> ScanAndReport(const std::vector<CleanTarget>& targets, bool isAdmin) {
    std::vector<ScanResult> results;
    ULONGLONG total = 0;
    g_totalSkipped = 0;
    int displayIdx = 0;

    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|         " << g->scanning << "         |\n";
    std::cout << "+--------------------------------------+\n\n";

    for (int i = 0; i < (int)targets.size(); i++) {
        auto& t = targets[i];
        if (t.requiresAdmin && !isAdmin) {
            results.push_back({t.name, 0, true, i});
            g_totalSkipped++;
            std::cout << "  (X) " << std::setw(28) << std::left << t.name
                      << g->skipped_admin << "\n";
            continue;
        }
        ULONGLONG size = ScanFolder(t.path);
        displayIdx++;
        results.push_back({t.name, size, false, i});
        total += size;
        std::cout << "  " << std::setw(2) << std::right << displayIdx << ". "
                  << std::setw(28) << std::left << t.name
                  << " " << FormatSize(size) << "\n";
    }

    std::cout << "\n  ------------------------------------\n";
    std::cout << "  " << g->total_cleanable << " : " << FormatSize(total) << "\n";
    std::cout << "  " << g->level_label << " : " << ClassifySize(total) << "\n";
    if (g_totalSkipped > 0)
        std::cout << "  " << g->skipped_count << " : " << g_totalSkipped << "\n";
    return results;
}

// ---------- Parse selection ----------
std::set<int> ParseSelection(const std::string& input, int maxNum) {
    std::set<int> indices;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t start = token.find_first_not_of(" \t");
        size_t end   = token.find_last_not_of(" \t");
        if (start == std::string::npos) continue;
        token = token.substr(start, end - start + 1);
        size_t dash = token.find('-');
        if (dash != std::string::npos && dash > 0 && dash < token.size() - 1) {
            try {
                int from = std::stoi(token.substr(0, dash));
                int to   = std::stoi(token.substr(dash + 1));
                if (from > to) std::swap(from, to);
                for (int i = from; i <= to; i++)
                    if (i >= 1 && i <= maxNum) indices.insert(i);
            } catch (...) {}
        } else {
            try {
                int n = std::stoi(token);
                if (n >= 1 && n <= maxNum) indices.insert(n);
            } catch (...) {}
        }
    }
    return indices;
}

// ---------- Ask cleanup mode ----------
int AskCleanMode(const std::vector<ScanResult>& results,
                 std::set<int>& selectedDisplayNums) {
    int cleanable = 0;
    ULONGLONG total = 0;
    for (auto& r : results) {
        if (!r.skipped && r.size > 0) { cleanable++; total += r.size; }
    }

    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|         " << g->summary_title << "         |\n";
    std::cout << "+--------------------------------------+\n";

    if (cleanable == 0) {
        std::cout << "|  " << g->no_junk << "  |\n";
        std::cout << "+--------------------------------------+\n";
        std::cout << "\n" << g->press_enter;
        std::cin.get();
        return -1;
    }

    std::cout << "|  " << g->summary_items << " : " << std::setw(2) << cleanable << "                         |\n";
    std::cout << "|  " << g->summary_size << " : " << FormatSize(total) << "                    |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|  " << g->summary_choose << "   |\n";
    std::cout << "|  " << g->summary_all << "                     |\n";
    std::cout << "|  " << g->summary_select << "               |\n";
    std::cout << "|  " << g->summary_no << "                   |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "\n" << g->prompt_mode;

    std::string input;
    std::getline(std::cin, input);

    if (input == "A" || input == "a") return 0;
    if (input == "N" || input == "n") return -1;
    if (input == "S" || input == "s") {
        std::cout << "\n  " << g->select_list_title << "\n\n";
        int displayIdx = 0;
        for (auto& r : results) {
            if (!r.skipped) {
                displayIdx++;
                std::cout << "    " << std::setw(2) << displayIdx << ". "
                          << std::setw(28) << std::left << r.name
                          << " " << FormatSize(r.size) << "\n";
            }
        }
        std::cout << "\n" << g->prompt_select;
        std::string selInput;
        std::getline(std::cin, selInput);
        selectedDisplayNums = ParseSelection(selInput, displayIdx);
        if (selectedDisplayNums.empty()) {
            std::cout << "\n  " << g->no_valid_sel << "\n";
            return -1;
        }
        ULONGLONG selTotal = 0;
        int selCount = 0;
        displayIdx = 0;
        std::cout << "\n  " << g->select_confirm_title << "\n";
        for (auto& r : results) {
            if (!r.skipped) {
                displayIdx++;
                if (selectedDisplayNums.count(displayIdx)) {
                    selCount++;
                    selTotal += r.size;
                    std::cout << "    > " << r.name << " (" << FormatSize(r.size) << ")\n";
                }
            }
        }
        std::cout << "\n  " << g->select_will_clean << " " << selCount
                  << " items, " << FormatSize(selTotal) << "\n";
        std::cout << g->select_confirm;
        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm == "Y" || confirm == "y") return 1;
    }
    return -1;
}

// ---------- Execute cleanup ----------
void ExecuteClean(const std::vector<CleanTarget>& targets, bool isAdmin,
                  const std::set<int>* filterDisplayNums,
                  const std::vector<ScanResult>& results) {
    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|       " << g->clean_title << "       |\n";
    std::cout << "+--------------------------------------+\n";

    std::set<int> targetIndices;
    if (filterDisplayNums) {
        int dispIdx = 0;
        for (int ri = 0; ri < (int)results.size(); ri++) {
            if (!results[ri].skipped) {
                dispIdx++;
                if (filterDisplayNums->count(dispIdx))
                    targetIndices.insert(results[ri].index);
            }
        }
    }

    ULONGLONG totalFreed = 0;
    for (int i = 0; i < (int)targets.size(); i++) {
        auto& t = targets[i];
        if (t.requiresAdmin && !isAdmin) continue;
        if (filterDisplayNums && !targetIndices.count(i)) continue;

        std::cout << "\n  " << g->cleaning << " " << t.name << " ...\n";
        ULONGLONG before = totalFreed;
        if (t.name.find("回收站") != std::string::npos ||
            t.name.find("Recycle") != std::string::npos) {
            std::wstring rp = L"C:\\$Recycle.Bin";
            std::error_code ec;
            if (fs::exists(fs::path(rp), ec)) {
                for (auto& uf : fs::directory_iterator(fs::path(rp), ec)) {
                    if (ec) { ec.clear(); continue; }
                    if (!fs::is_directory(uf)) continue;
                    std::wstring n = uf.path().filename().wstring();
                    if (n == L"S-1-5-18" || n == L"S-1-5-19" || n == L"S-1-5-20") continue;
                    totalFreed += DeleteFolder(uf.path().wstring());
                }
            }
            SHEmptyRecycleBinW(nullptr, nullptr,
                SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        } else {
            totalFreed += DeleteFolder(t.path);
        }
        ULONGLONG diff = totalFreed - before;
        std::cout << "    -> " << g->freed << " " << FormatSize(diff) << "\n";
    }

    g_totalDeleted = totalFreed;
    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|       " << g->done_title << "       |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|  " << g->done_freed << " : " << std::setw(12) << FormatSize(g_totalDeleted) << "  |\n";
    std::cout << "|  " << g->done_failed << " : " << std::setw(12) << g_totalFailed << "  |\n";
    std::cout << "+--------------------------------------+\n";
}

// ---------- Main ----------
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    ChooseLanguage();
    bool isAdmin = IsRunningAsAdmin();

    if (!ShowDisclaimer(isAdmin)) {
        std::cout << "\n" << g->cancelled;
        std::cin.get();
        return 0;
    }

    wchar_t userProfile[MAX_PATH];
    wchar_t localAppData[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    std::wstring profile(userProfile);
    std::wstring local(localAppData);

    std::vector<CleanTarget> targets = {
        {g->name_user_temp,       profile + L"\\AppData\\Local\\Temp", true, false},
        {g->name_chrome_cache,    local + L"\\Google\\Chrome\\User Data\\Default\\Cache", true, false},
        {g->name_chrome_code,     local + L"\\Google\\Chrome\\User Data\\Default\\Code Cache", true, false},
        {g->name_edge_cache,      local + L"\\Microsoft\\Edge\\User Data\\Default\\Cache", true, false},
        {g->name_edge_code,       local + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache", true, false},
        {g->name_firefox_cache,   local + L"\\Mozilla\\Firefox\\Profiles", true, false},
        {g->name_thumb_cache,     profile + L"\\AppData\\Local\\Microsoft\\Windows\\Explorer", true, false},
        {g->name_win_temp,        L"C:\\Windows\\Temp", true, true},
        {g->name_recycle,         L"C:\\$Recycle.Bin", true, true},
        {g->name_prefetch,        L"C:\\Windows\\Prefetch", true, true},
        {g->name_win_update,      L"C:\\Windows\\SoftwareDistribution\\Download", true, true},
        {g->name_wer,             L"C:\\ProgramData\\Microsoft\\Windows\\WER", true, true},
        {g->name_win_logs,        L"C:\\Windows\\Logs", true, true},
    };

    auto results = ScanAndReport(targets, isAdmin);

    std::set<int> selectedNums;
    int mode = AskCleanMode(results, selectedNums);
    if (mode == -1) {
        std::cout << "\n" << g->cancelled;
        std::cin.get();
        return 0;
    }

    if (mode == 0)
        ExecuteClean(targets, isAdmin, nullptr, results);
    else
        ExecuteClean(targets, isAdmin, &selectedNums, results);

    std::cout << "\n" << g->press_enter;
    std::cin.get();
    return 0;
}
