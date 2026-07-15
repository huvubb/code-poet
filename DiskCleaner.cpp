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
    int         index;  // 对应 targets 中的原始索引
};

// ---------- 显示免责声明 ----------
bool ShowDisclaimer(bool isAdmin) {
    system("cls");
    std::cout << R"(
╔══════════════════════════════════════════════════════╗
║            C盘垃圾清理工具  v1.3.0                    ║
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
    int displayIdx = 0;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║         📊 正在扫描垃圾文件...           ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    for (int i = 0; i < (int)targets.size(); i++) {
        auto& t = targets[i];
        if (t.requiresAdmin && !isAdmin) {
            results.push_back({t.name, 0, true, i});
            g_totalSkipped++;
            std::cout << "  ⏭  " << std::setw(28) << std::left << t.name
                      << " 跳过（需管理员权限）\n";
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

    std::cout << "\n  ─────────────────────────────────────\n";
    std::cout << "  📦 总计可清理 : " << FormatSize(total) << "\n";
    std::cout << "  📊 垃圾量评级 : " << ClassifySize(total) << "\n";
    if (g_totalSkipped > 0) {
        std::cout << "  ⏭  跳过 " << g_totalSkipped << " 项（需管理员权限）\n";
    }

    return results;
}

// ---------- 解析用户输入的选择编号 ----------
std::set<int> ParseSelection(const std::string& input, int maxNum) {
    std::set<int> indices;  // 这里存的是用户看到的编号(1-based)
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // 去掉首尾空格
        size_t start = token.find_first_not_of(" \t");
        size_t end   = token.find_last_not_of(" \t");
        if (start == std::string::npos) continue;
        token = token.substr(start, end - start + 1);

        // 处理范围如 "3-5"
        size_t dash = token.find('-');
        if (dash != std::string::npos && dash > 0 && dash < token.size() - 1) {
            try {
                int from = std::stoi(token.substr(0, dash));
                int to   = std::stoi(token.substr(dash + 1));
                if (from > to) std::swap(from, to);
                for (int i = from; i <= to; i++) {
                    if (i >= 1 && i <= maxNum) indices.insert(i);
                }
            } catch (...) { continue; }
        } else {
            try {
                int n = std::stoi(token);
                if (n >= 1 && n <= maxNum) indices.insert(n);
            } catch (...) { continue; }
        }
    }
    return indices;
}

// ---------- 询问清理方式 ----------
// 返回: 0=全部, 1=选择, -1=退出
// 如果选择模式，selectedDisplayNums 存储用户选择的显示编号
int AskCleanMode(const std::vector<ScanResult>& results,
                 std::set<int>& selectedDisplayNums) {
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
        return -1;
    }

    std::cout << "║  可清理项目 : " << std::setw(2) << cleanable << " 项"
              << "                     ║\n";
    std::cout << "║  总计大小   : " << FormatSize(total)
              << "                  ║\n";
    std::cout << "╠══════════════════════════════════════════╣\n";
    std::cout << "║  请选择清理方式：                        ║\n";
    std::cout << "║  [A] 全部清理                            ║\n";
    std::cout << "║  [S] 自定义选择（输入编号）              ║\n";
    std::cout << "║  [N] 不清理，安全退出                    ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "\n👉 请输入 A / S / N: ";

    std::string input;
    std::getline(std::cin, input);

    if (input == "A" || input == "a") return 0;
    if (input == "N" || input == "n") return -1;
    if (input == "S" || input == "s") {
        // 显示编号列表供选择
        std::cout << "\n  可清理项目列表（输入编号，逗号分隔，如 1,3-5,7）:\n\n";
        int displayIdx = 0;
        for (auto& r : results) {
            if (!r.skipped) {
                displayIdx++;
                std::cout << "    " << std::setw(2) << displayIdx << ". "
                          << std::setw(28) << std::left << r.name
                          << " " << FormatSize(r.size) << "\n";
            }
        }
        std::cout << "\n👉 请输入要清理的编号（如 1,3-5）: ";

        std::string selInput;
        std::getline(std::cin, selInput);

        selectedDisplayNums = ParseSelection(selInput, displayIdx);
        if (selectedDisplayNums.empty()) {
            std::cout << "\n  ❌ 没有有效的选择，已取消。\n";
            return -1;
        }

        // 显示确认
        ULONGLONG selTotal = 0;
        int selCount = 0;
        displayIdx = 0;
        std::cout << "\n  已选择清理以下项目:\n";
        for (auto& r : results) {
            if (!r.skipped) {
                displayIdx++;
                if (selectedDisplayNums.count(displayIdx)) {
                    selCount++;
                    selTotal += r.size;
                    std::cout << "    ✓ " << r.name << " (" << FormatSize(r.size) << ")\n";
                }
            }
        }
        std::cout << "\n  📦 本次将清理 " << selCount << " 项，共 " << FormatSize(selTotal) << "\n";
        std::cout << "👉 确认清理？输入 Y 执行，其他任意键取消: ";

        std::string confirm;
        std::getline(std::cin, confirm);
        if (confirm == "Y" || confirm == "y") return 1;
    }
    return -1;
}

// ---------- 执行清理（支持筛选） ----------
void ExecuteClean(const std::vector<CleanTarget>& targets, bool isAdmin,
                  const std::set<int>* filterDisplayNums,
                  const std::vector<ScanResult>& results) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║         🧹 开始清理垃圾文件...           ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";

    // 构建显示编号 -> 结果索引 的映射
    std::set<int> targetIndices;
    if (filterDisplayNums) {
        int dispIdx = 0;
        for (int ri = 0; ri < (int)results.size(); ri++) {
            if (!results[ri].skipped) {
                dispIdx++;
                if (filterDisplayNums->count(dispIdx)) {
                    targetIndices.insert(results[ri].index);
                }
            }
        }
    }

    ULONGLONG totalFreed = 0;
    for (int i = 0; i < (int)targets.size(); i++) {
        auto& t = targets[i];
        if (t.requiresAdmin && !isAdmin) continue;
        // 如果是选择模式，跳过未选中的
        if (filterDisplayNums && !targetIndices.count(i)) continue;

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

    // 构建清理目标
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

    // 第二步：扫描并生成报告（带编号）
    auto results = ScanAndReport(targets, isAdmin);

    // 第三步：询问清理方式
    std::set<int> selectedNums;
    int mode = AskCleanMode(results, selectedNums);

    if (mode == -1) {
        std::cout << "\n已取消清理。按回车退出...";
        std::cin.get();
        return 0;
    }

    // 第四步：执行清理
    if (mode == 0) {
        // 全部清理
        ExecuteClean(targets, isAdmin, nullptr, results);
    } else if (mode == 1) {
        // 选择清理
        ExecuteClean(targets, isAdmin, &selectedNums, results);
    }

    std::cout << "\n按回车退出...";
    std::cin.get();
    return 0;
}
