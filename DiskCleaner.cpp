#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================
//  免责声明
// ============================================================
//  本软件按"原样"提供，不附带任何形式的明示或暗示担保。
//  使用本软件即表示您同意自行承担所有风险。
// ============================================================

struct CleanTarget {
    std::string name;
    std::string path;
    bool        isFolder;
    bool        requiresAdmin;
};

ULONGLONG g_totalDeleted = 0;
ULONGLONG g_totalFailed  = 0;
ULONGLONG g_totalSkipped = 0;

std::string FormatSize(ULONGLONG bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int idx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %s", val, units[idx]);
    return buf;
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token, TokenElevation, &elevation, size, &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return isAdmin != FALSE;
}

// Convert UTF-8 string to wide string for filesystem ops
std::wstring ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
    return result;
}

ULONGLONG DeleteFolder(const std::string& folder) {
    ULONGLONG freed = 0;
    std::error_code ec;
    fs::path fp = ToWide(folder);
    for (auto& entry : fs::recursive_directory_iterator(fp, ec)) {
        if (ec) { ec.clear(); continue; }
        if (fs::is_regular_file(entry)) {
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

ULONGLONG EmptyRecycleBin() {
    ULONGLONG freed = 0;
    std::string recyclePath = "C:\\$Recycle.Bin";
    std::error_code ec;
    fs::path rp = ToWide(recyclePath);
    if (fs::exists(rp, ec)) {
        for (auto& userFolder : fs::directory_iterator(rp, ec)) {
            if (ec) { ec.clear(); continue; }
            if (!fs::is_directory(userFolder)) continue;
            std::wstring name = userFolder.path().filename().wstring();
            if (name == L"S-1-5-18" || name == L"S-1-5-19" || name == L"S-1-5-20") continue;
            freed += DeleteFolder(recyclePath);  // not ideal but ok
        }
    }
    SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return freed;
}

void ExecuteClean(const std::vector<CleanTarget>& targets, bool isAdmin) {
    std::error_code ec;
    for (auto& t : targets) {
        if (t.requiresAdmin && !isAdmin) {
            std::cout << "\n[跳过] " << t.name << "（需要管理员权限）\n";
            g_totalSkipped++;
            continue;
        }
        std::cout << "\n[扫描] " << t.name << "\n  路径: " << t.path << "\n";
        fs::path fp = ToWide(t.path);
        if (!fs::exists(fp, ec) && t.isFolder) {
            std::cout << "  -> 路径不存在，跳过。\n";
            continue;
        }
        if (ec) { ec.clear(); }
        ULONGLONG before = g_totalDeleted;
        if (t.name.find("回收站") != std::string::npos) {
            g_totalDeleted += EmptyRecycleBin();
        } else {
            g_totalDeleted += DeleteFolder(t.path);
        }
        ULONGLONG diff = g_totalDeleted - before;
        std::cout << "  -> 已清理: " << FormatSize(diff) << "\n";
    }
}

bool ShowDisclaimer(bool isAdmin) {
    system("cls");
    std::cout << R"(
+------------------------------------------------------+
|            C盘垃圾清理工具  v1.0.0                     |
+------------------------------------------------------+
|                   ** 免责声明 **                       |
|                                                      |
|  本软件按"原样"提供，不附带任何形式的担保。          |
|  使用本软件即表示您同意自行承担所有风险。            |
|  开发者对因使用本软件而导致的数据丢失、               |
|  系统损坏或其他损失概不负责。                         |
|                                                      |
|  清理范围：Temp / 回收站 / 浏览器缓存 / Prefetch     |
|           更新缓存 / 错误报告 / 日志 / 缩略图        |
+------------------------------------------------------+
)";

    if (isAdmin) {
        std::cout << "\n  [管理员模式] 完整清理 - 所有项目可用。\n";
    } else {
        std::cout << "\n  [普通用户模式] 仅清理无需管理员权限的项目。\n";
        std::cout << "  以管理员身份运行可获得完整清理能力。\n";
    }

    std::cout << "\n输入 YES 确认继续清理，其他任意键退出: ";
    std::string input;
    std::getline(std::cin, input);
    return (input == "YES" || input == "yes" || input == "Yes");
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    bool isAdmin = IsRunningAsAdmin();

    if (!ShowDisclaimer(isAdmin)) {
        std::cout << "\n已取消清理。按回车退出...";
        std::cin.get();
        return 0;
    }

    char userProfile[MAX_PATH];
    char localAppData[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", userProfile, MAX_PATH);
    GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH);
    std::string profile(userProfile);
    std::string local(localAppData);

    std::vector<CleanTarget> targets = {
        // ---- 无需管理员权限 ----
        {"用户 Temp",              profile + "\\AppData\\Local\\Temp", true, false},
        {"Chrome 缓存",            local + "\\Google\\Chrome\\User Data\\Default\\Cache", true, false},
        {"Chrome Code Cache",      local + "\\Google\\Chrome\\User Data\\Default\\Code Cache", true, false},
        {"Edge 缓存",              local + "\\Microsoft\\Edge\\User Data\\Default\\Cache", true, false},
        {"Edge Code Cache",        local + "\\Microsoft\\Edge\\User Data\\Default\\Code Cache", true, false},
        {"Firefox 缓存",           local + "\\Mozilla\\Firefox\\Profiles", true, false},
        {"缩略图缓存",             profile + "\\AppData\\Local\\Microsoft\\Windows\\Explorer", true, false},

        // ---- 需要管理员权限 ----
        {"Windows Temp",           "C:\\Windows\\Temp", true, true},
        {"回收站",                 "C:\\$Recycle.Bin", true, true},
        {"预读取文件 (Prefetch)",   "C:\\Windows\\Prefetch", true, true},
        {"Windows 更新缓存",        "C:\\Windows\\SoftwareDistribution\\Download", true, true},
        {"Windows 错误报告",        "C:\\ProgramData\\Microsoft\\Windows\\WER", true, true},
        {"Windows 日志",           "C:\\Windows\\Logs", true, true},
    };

    std::cout << "\n开始分析清理目标，共 " << targets.size() << " 项。\n";
    ExecuteClean(targets, isAdmin);

    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|         清理完成 - 汇总报告           |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|  已清理空间 : " << std::setw(14) << FormatSize(g_totalDeleted) << "  |\n";
    std::cout << "|  失败项     : " << std::setw(14) << g_totalFailed << "  |\n";
    std::cout << "|  跳过项     : " << std::setw(14) << g_totalSkipped << "  |\n";
    std::cout << "+--------------------------------------+\n";

    std::cout << "\n按回车退出...";
    std::cin.get();
    return 0;
}
