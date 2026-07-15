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
    if (bytes < 500ULL * 1024 * 1024)       return "轻度   (< 500 MB)";
    if (bytes < 2ULL * 1024 * 1024 * 1024)  return "中等   (500 MB ~ 2 GB)";
    if (bytes < 5ULL * 1024 * 1024 * 1024)  return "较多   (2 GB ~ 5 GB)";
    if (bytes < 10ULL * 1024 * 1024 * 1024) return "大量   (5 GB ~ 10 GB)";
    return                                      "严重   (> 10 GB)";
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
};

// ---------- 显示免责声明 ----------
bool ShowDisclaimer(bool isAdmin) {
    system("cls");
    std::cout << R"(
╔══════════════════════════════════════════════════════╗
║            C盘垃圾清理工具  v1.2.0                    ║
╠══════════════════════════════════════════════════════╣
║                 ⚠️  免 责 声 明  ⚠️                    ║
║                                                      ║
║  本软件按"原样"提供，不附带任何形式的担保。          ║
║  使用本软件即表示您同意自行承担所有风险。            ║
║  开发者对因使用本软件导致的数据丢失、                 ║
║  系统损坏或其他损失概不负责。                         ║
║                                                      ║
║  清理范围：Temp / 回收站 / 浏览器缓存 / Prefetch     ║
║           更新缓存 / 错误报告 / 日志 / 缩略图        ║
╚══════════════════════════════════════════════════════╝
)";

    if (isAdmin) {
        std::cout << "\n  🔧 [管理员模式] 可清理全部 13 项\n";
    } else {
        std::cout << "\n  👤 [普通用户模式] 可清理 7 项（跳过系统目录）\n";
        std::cout << "    💡 以管理员身份运行可解锁全部清理能力\n";
    }

    std::cout << "\n👉 输入 YES 开始扫描垃圾文件，其他任意键退出: ";
    std::string input;
    std::getline(std::cin, input);
    return (input == "YES" || input == "yes" || input == "Yes");
}

// ---------- 扫描并生成报告 ----------
std::vector<ScanResult> ScanAndReport(const std::vector<CleanTarget>& targets, bool isAdmin) {
    std::vector<ScanResult> results;
    ULONGLONG total = 0;
    g_totalSkipped = 0;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║         📊 正在扫描垃圾文件...           ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    for (auto& t : targets) {
        if (t.requiresAdmin && !isAdmin) {
            results.push_back({t.name, 0, true});
            g_totalSkipped++;
            std::cout << "  ⏭  " << std::setw(28) << std::left << t.name
                      << " 跳过（需管理员权限）\n";
            continue;
        }
        ULONGLONG size = ScanFolder(t.path);
        results.push_back({t.name, size, false});
        total += size;
        std::cout << "  📁 " << std::setw(28) << std::left << t.name
                  << " " << FormatSize(size) << "\n";
    }

    std::cout << "\n  ─────────────────────────────────────\n";
    std::cout << "  📦 总计可清理 : " << FormatSize(total) << "\n";
    std::cout << "  📊 垃圾量评级 : " << ClassifySize(total) << "\n";
    if (g_totalSkipped > 0) {
        std::cout << "  ⏭  跳过 " << g_totalSkipped << " 项（需管理员权限）\n";
    }

    return results;
}

// ---------- 询问是否清理 ----------
bool AskToClean(const std::vector<ScanResult>& results) {
    // 统计有效清理项
    int cleanable = 0;
    ULONGLONG total = 0;
    for (auto& r : results) {
        if (!r.skipped && r.size > 0) {
            cleanable++;
            total += r.size;
        }
    }

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║           📋 扫描报告汇总                ║\n";
    std::cout << "╠══════════════════════════════════════════╣\n";

    if (cleanable == 0) {
        std::cout << "║  没有发现可清理的垃圾文件。              ║\n";
        std::cout << "╚══════════════════════════════════════════╝\n";
        std::cout << "\n按回车退出...";
        std::cin.get();
        return false;
    }

    std::cout << "║  可清理项目 : " << std::setw(2) << cleanable << " 项"
              << "                     ║\n";
    std::cout << "║  总计大小   : " << FormatSize(total)
              << "                  ║\n";
    std::cout << "╠══════════════════════════════════════════╣\n";
    std::cout << "║  是否清理以上垃圾文件？                  ║\n";
    std::cout << "║  [Y] 是，立即清理                        ║\n";
    std::cout << "║  [N] 否，安全退出                        ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "\n👉 请输入 Y 或 N: ";

    std::string input;
    std::getline(std::cin, input);
    return (input == "Y" || input == "y");
}

// ---------- 执行清理 ----------
void ExecuteClean(const std::vector<CleanTarget>& targets, bool isAdmin) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║         🧹 开始清理垃圾文件...           ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";

    ULONGLONG totalFreed = 0;
    for (auto& t : targets) {
        if (t.requiresAdmin && !isAdmin) continue;
        std::cout << "\n  清理 " << t.name << " ...\n";
        ULONGLONG before = totalFreed;
        if (t.name.find("回收站") != std::string::npos) {
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
        std::cout << "    ✅ 释放 " << FormatSize(diff) << "\n";
    }

    g_totalDeleted = totalFreed;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║         ✅ 清理完成 - 汇总报告           ║\n";
    std::cout << "╠══════════════════════════════════════════╣\n";
    std::cout << "║  已清理空间 : " << std::setw(14) << FormatSize(g_totalDeleted) << "  ║\n";
    std::cout << "║  失败项     : " << std::setw(14) << g_totalFailed << "  ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
}

// ---------- 主入口 ----------
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    bool isAdmin = IsRunningAsAdmin();

    // 第一步：免责声明
    if (!ShowDisclaimer(isAdmin)) {
        std::cout << "\n已取消。按回车退出...";
        std::cin.get();
        return 0;
    }

    // 构建清理目标（宽字符路径，支持中文）
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

    // 第二步：扫描并生成报告
    auto results = ScanAndReport(targets, isAdmin);

    // 第三步：询问用户是否清理
    if (!AskToClean(results)) {
        std::cout << "\n已取消清理。按回车退出...";
        std::cin.get();
        return 0;
    }

    // 第四步：执行清理
    ExecuteClean(targets, isAdmin);

    std::cout << "\n按回车退出...";
    std::cin.get();
    return 0;
}
