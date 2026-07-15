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
    std::string  name;       // 显示名（UTF-8）
    std::wstring path;       // 路径（宽字符，支持中文）
    bool         isFolder;
    bool         requiresAdmin;
};

ULONGLONG g_totalDeleted  = 0;
ULONGLONG g_totalFailed   = 0;
int       g_totalSkipped  = 0;

// ---------- 格式化字节 ----------
std::string FormatSize(ULONGLONG bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int idx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %s", val, units[idx]);
    return buf;
}

// ---------- 垃圾量分级 ----------
const char* ClassifySize(ULONGLONG bytes) {
    if (bytes < 500ULL * 1024 * 1024)       return "轻度   (< 500 MB)";
    if (bytes < 2ULL * 1024 * 1024 * 1024)  return "中等   (500 MB ~ 2 GB)";
    if (bytes < 5ULL * 1024 * 1024 * 1024)  return "较多   (2 GB ~ 5 GB)";
    if (bytes < 10ULL * 1024 * 1024 * 1024) return "大量   (5 GB ~ 10 GB)";
    return                                      "严重   (> 10 GB)";
}

// ---------- 管理员检测 ----------
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

// ---------- 扫描文件夹大小（不删除） ----------
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

// ---------- 删除文件夹 ----------
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
            if (ec) {
                g_totalFailed++;
            } else {
                g_totalDeleted++;
            }
        }
    }
    // 删空目录
    for (auto& entry : fs::recursive_directory_iterator(fp, ec)) {
        if (ec) { ec.clear(); continue; }
        fs::remove(entry, ec);
    }
    fs::remove(fp, ec);
    g_totalDeleted += freed;
    return freed;
}

// ---------- 扫描阶段 ----------
struct ScanResult {
    std::string name;
    ULONGLONG   size;
    bool        skipped;
};
std::vector<ScanResult> g_scanResults;
ULONGLONG g_totalScanned = 0;

void ScanTargets(const std::vector<CleanTarget>& targets, bool isAdmin) {
    g_scanResults.clear();
    g_totalScanned = 0;

    std::cout << "\n========================================\n";
    std::cout << "         正在扫描垃圾文件...\n";
    std::cout << "========================================\n";

    for (auto& t : targets) {
        if (t.requiresAdmin && !isAdmin) {
            g_scanResults.push_back({t.name, 0, true});
            g_totalSkipped++;
            continue;
        }
        ULONGLONG size = ScanFolder(t.path);
        g_scanResults.push_back({t.name, size, false});
        g_totalScanned += size;

        std::cout << "  " << std::setw(25) << std::left << t.name
                  << " : " << std::setw(12) << FormatSize(size) << "\n";
    }

    std::cout << "----------------------------------------\n";
    std::cout << "  总计可清理空间 : " << FormatSize(g_totalScanned) << "\n";
    std::cout << "  垃圾量评级     : " << ClassifySize(g_totalScanned) << "\n";
    std::cout << "  跳过项         : " << g_totalSkipped << " 项（需管理员权限）\n";
    std::cout << "========================================\n";
}

// ---------- 执行清理 ----------
void ExecuteClean(const std::vector<CleanTarget>& targets, bool isAdmin) {
    ULONGLONG freed = 0;
    for (auto& t : targets) {
        if (t.requiresAdmin && !isAdmin) continue;
        std::cout << "\n[清理] " << t.name << " ...\n";
        ULONGLONG before = freed;
        if (t.name.find("回收站") != std::string::npos) {
            std::wstring rp = L"C:\\$Recycle.Bin";
            std::error_code ec;
            if (fs::exists(fs::path(rp), ec)) {
                for (auto& uf : fs::directory_iterator(fs::path(rp), ec)) {
                    if (ec) { ec.clear(); continue; }
                    if (!fs::is_directory(uf)) continue;
                    std::wstring n = uf.path().filename().wstring();
                    if (n == L"S-1-5-18" || n == L"S-1-5-19" || n == L"S-1-5-20") continue;
                    freed += DeleteFolder(uf.path().wstring());
                }
            }
            SHEmptyRecycleBinW(nullptr, nullptr,
                SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        } else {
            freed += DeleteFolder(t.path);
        }
        ULONGLONG diff = freed - before;
        std::cout << "  -> 已释放: " << FormatSize(diff) << "\n";
    }
}

// ---------- 免责声明 ----------
bool ShowDisclaimer(bool isAdmin) {
    system("cls");
    std::cout << R"(
+------------------------------------------------------+
|            C盘垃圾清理工具  v1.1.0                     |
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
        std::cout << "\n  [管理员模式] 完整清理\n";
    } else {
        std::cout << "\n  [普通用户模式] 仅清理用户目录垃圾\n";
        std::cout << "  以管理员身份运行可清理系统目录。\n";
    }

    std::cout << "\n输入 YES 开始扫描，其他任意键退出: ";
    std::string input;
    std::getline(std::cin, input);
    return (input == "YES" || input == "yes" || input == "Yes");
}

// ---------- 主入口 ----------
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    bool isAdmin = IsRunningAsAdmin();

    if (!ShowDisclaimer(isAdmin)) {
        std::cout << "\n已取消，按回车退出...";
        std::cin.get();
        return 0;
    }

    // 获取路径（宽字符版本，支持中文）
    wchar_t userProfile[MAX_PATH];
    wchar_t localAppData[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    std::wstring profile(userProfile);
    std::wstring local(localAppData);

    std::vector<CleanTarget> targets = {
        // ---- 无需管理员权限 ----
        {"用户 Temp",              profile + L"\\AppData\\Local\\Temp", true, false},
        {"Chrome 缓存",            local + L"\\Google\\Chrome\\User Data\\Default\\Cache", true, false},
        {"Chrome Code Cache",      local + L"\\Google\\Chrome\\User Data\\Default\\Code Cache", true, false},
        {"Edge 缓存",              local + L"\\Microsoft\\Edge\\User Data\\Default\\Cache", true, false},
        {"Edge Code Cache",        local + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache", true, false},
        {"Firefox 缓存",           local + L"\\Mozilla\\Firefox\\Profiles", true, false},
        {"缩略图缓存",             profile + L"\\AppData\\Local\\Microsoft\\Windows\\Explorer", true, false},

        // ---- 需要管理员权限 ----
        {"Windows Temp",           L"C:\\Windows\\Temp", true, true},
        {"回收站",                 L"C:\\$Recycle.Bin", true, true},
        {"预读取文件 (Prefetch)",   L"C:\\Windows\\Prefetch", true, true},
        {"Windows 更新缓存",        L"C:\\Windows\\SoftwareDistribution\\Download", true, true},
        {"Windows 错误报告",        L"C:\\ProgramData\\Microsoft\\Windows\\WER", true, true},
        {"Windows 日志",           L"C:\\Windows\\Logs", true, true},
    };

    // ---- 第一阶段：扫描 ----
    ScanTargets(targets, isAdmin);

    // ---- 询问是否执行清理 ----
    std::cout << "\n输入 DELETE 确认删除以上垃圾文件，其他任意键退出: ";
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "DELETE") {
        std::cout << "\n已取消清理，按回车退出...";
        std::cin.get();
        return 0;
    }

    // ---- 第二阶段：清理 ----
    g_totalDeleted = 0;
    g_totalFailed = 0;
    ExecuteClean(targets, isAdmin);

    // ---- 汇总报告 ----
    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|         清理完成 - 汇总报告           |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|  已清理空间 : " << std::setw(14) << FormatSize(g_totalDeleted) << "  |\n";
    std::cout << "|  失败项     : " << std::setw(14) << g_totalFailed << "  |\n";
    std::cout << "+--------------------------------------+\n";

    std::cout << "\n按回车退出...";
    std::cin.get();
    return 0;
}
