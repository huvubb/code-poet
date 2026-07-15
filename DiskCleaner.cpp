#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================
//  免责声明 / Disclaimer
// ============================================================
// 本软件按"原样"提供，不附带任何形式的明示或暗示担保。
// 使用本软件即表示您同意自行承担所有风险。开发者对因使
// 用本软件而导致的任何数据丢失、系统损坏或其他损失概不
// 负责。请在运行清理前关闭不必要的程序并备份重要数据。
// ============================================================

struct CleanTarget {
    std::wstring name;
    std::wstring path;
    bool        isFolder;   // true = 文件夹, false = 通配文件
};

// ---------- 全局统计 ----------
ULONGLONG g_totalDeleted = 0;
ULONGLONG g_totalFailed  = 0;
ULONGLONG g_totalSkipped = 0;   // 受保护/拒绝访问

// ---------- 格式化字节 ----------
std::wstring FormatSize(ULONGLONG bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int idx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && idx < 4) {
        val /= 1024.0;
        ++idx;
    }
    wchar_t buf[64];
    swprintf_s(buf, L"%.2f %s", val, units[idx]);
    return buf;
}

// ---------- 递归删除文件夹 ----------
ULONGLONG DeleteFolder(const fs::path& folder) {
    ULONGLONG freed = 0;
    std::error_code ec;
    for (auto& entry : fs::recursive_directory_iterator(folder, ec)) {
        if (ec) { ec.clear(); continue; }
        if (fs::is_regular_file(entry)) {
            ULONGLONG sz = fs::file_size(entry, ec);
            if (!ec) freed += sz;
            fs::remove(entry, ec);
            if (ec) g_totalFailed++;
        }
    }
    // 删空目录
    for (auto& entry : fs::recursive_directory_iterator(folder, ec)) {
        if (ec) { ec.clear(); continue; }
        fs::remove(entry, ec);
    }
    fs::remove(folder, ec);
    return freed;
}

// ---------- 删除匹配通配符的文件 ----------
ULONGLONG DeleteWildcard(const fs::path& root, const std::wstring& pattern) {
    ULONGLONG freed = 0;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(root, ec)) {
        if (ec) { ec.clear(); continue; }
        if (!fs::is_regular_file(entry)) continue;
        std::wstring name = entry.path().filename().wstring();
        // 简单通配: *.ext 匹配
        if (pattern.size() > 2 && pattern[0] == L'*' && pattern[1] == L'.') {
            std::wstring ext = pattern.substr(1);  // .ext
            if (name.size() >= ext.size() &&
                _wcsicmp(name.c_str() + name.size() - ext.size(), ext.c_str()) == 0) {
                ULONGLONG sz = fs::file_size(entry, ec);
                if (!ec) freed += sz;
                fs::remove(entry, ec);
                if (ec) g_totalFailed++;
            }
        }
    }
    return freed;
}

// ---------- 清空回收站 ----------
ULONGLONG EmptyRecycleBin() {
    ULONGLONG freed = 0;
    // 估算回收站大小（无法精确）
    std::wstring recyclePath = L"C:\\$Recycle.Bin";
    std::error_code ec;
    if (fs::exists(recyclePath, ec)) {
        for (auto& userFolder : fs::directory_iterator(recyclePath, ec)) {
            if (ec) { ec.clear(); continue; }
            if (!fs::is_directory(userFolder)) continue;
            std::wstring name = userFolder.path().filename().wstring();
            if (name == L"S-1-5-18" || name == L"S-1-5-19" || name == L"S-1-5-20") continue;
            freed += DeleteFolder(userFolder.path());
        }
    }
    // 调用 SHEmptyRecycleBin
    SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return freed;
}

// ---------- 执行清理 ----------
void ExecuteClean(const std::vector<CleanTarget>& targets) {
    std::error_code ec;

    for (auto& t : targets) {
        std::wcout << L"\n[扫描] " << t.name << L"\n  路径: " << t.path << L"\n";

        if (!fs::exists(t.path, ec) && t.isFolder) {
            std::wcout << L"  -> 路径不存在，跳过。\n";
            continue;
        }
        if (ec) { ec.clear(); }

        ULONGLONG before = g_totalDeleted;

        if (t.isFolder) {
            if (t.name.find(L"回收站") != std::wstring::npos) {
                g_totalDeleted += EmptyRecycleBin();
            } else {
                g_totalDeleted += DeleteFolder(t.path);
            }
        } else {
            // 通配文件
            g_totalDeleted += DeleteWildcard(t.path, t.path);
        }

        ULONGLONG diff = g_totalDeleted - before;
        std::wcout << L"  -> 清理: " << FormatSize(diff) << L"\n";
    }
}

// ---------- 显示免责声明并确认 ----------
bool ShowDisclaimer() {
    system("cls");
    std::wcout << L"\n";
    std::wcout << L"╔══════════════════════════════════════════════════════╗\n";
    std::wcout << L"║            C盘垃圾清理工具  v1.0                      ║\n";
    std::wcout << L"╠══════════════════════════════════════════════════════╣\n";
    std::wcout << L"║                    ⚠ 免责声明 ⚠                       ║\n";
    std::wcout << L"║                                                      ║\n";
    std::wcout << L"║  本软件按\"原样\"提供，不附带任何形式的明示或暗示担保。 ║\n";
    std::wcout << L"║  使用本软件即表示您同意自行承担所有风险。            ║\n";
    std::wcout << L"║  开发者对因使用本软件而导致的任何数据丢失、           ║\n";
    std::wcout << L"║  系统损坏或其他损失概不负责。                         ║\n";
    std::wcout << L"║                                                      ║\n";
    std::wcout << L"║  清理范围包括但不限于：                               ║\n";
    std::wcout << L"║  · 临时文件 (Temp)                                   ║\n";
    std::wcout << L"║  · 回收站                                            ║\n";
    std::wcout << L"║  · Windows 更新缓存                                  ║\n";
    std::wcout << L"║  · 浏览器缓存                                        ║\n";
    std::wcout << L"║  · 系统日志 & 错误报告                               ║\n";
    std::wcout << L"║  · 预读取文件 (Prefetch)                             ║\n";
    std::wcout << L"║                                                      ║\n";
    std::wcout << L"║  请关闭不必要的程序后再继续！                         ║\n";
    std::wcout << L"╚══════════════════════════════════════════════════════╝\n\n";
    std::wcout << L"输入 YES 确认继续清理，输入其他任意键退出: ";

    std::wstring input;
    std::getline(std::wcin, input);

    if (input == L"YES" || input == L"yes" || input == L"Yes") {
        return true;
    }
    return false;
}

// ---------- 主入口 ----------
int wmain(int argc, wchar_t* argv[]) {
    // 设置控制台编码支持中文
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::locale::global(std::locale(""));

    if (!ShowDisclaimer()) {
        std::wcout << L"\n已取消清理，程序退出。按任意键关闭...";
        std::wcin.get();
        return 0;
    }

    // 获取当前用户名目录
    wchar_t userProfile[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);
    std::wstring profile(userProfile);
    std::wstring localAppData;
    {
        wchar_t buf[MAX_PATH];
        GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
        localAppData = buf;
    }

    // 定义清理目标
    std::vector<CleanTarget> targets = {
        // ---- 用户临时文件 ----
        { L"用户 Temp",  profile + L"\\AppData\\Local\\Temp", true },
        { L"Windows Temp", L"C:\\Windows\\Temp", true },

        // ---- 回收站 ----
        { L"回收站", L"C:\\$Recycle.Bin", true },

        // ---- Prefetch ----
        { L"预读取文件 (Prefetch)", L"C:\\Windows\\Prefetch", true },

        // ---- Windows 更新缓存 ----
        { L"Windows 更新缓存", L"C:\\Windows\\SoftwareDistribution\\Download", true },

        // ---- 错误报告 ----
        { L"Windows 错误报告", L"C:\\ProgramData\\Microsoft\\Windows\\WER", true },

        // ---- 浏览器缓存 ----
        { L"Chrome 缓存",   localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache", true },
        { L"Chrome Code Cache", localAppData + L"\\Google\\Chrome\\User Data\\Default\\Code Cache", true },
        { L"Edge 缓存",     localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache", true },
        { L"Edge Code Cache", localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache", true },
        { L"Firefox 缓存",  localAppData + L"\\Mozilla\\Firefox\\Profiles", true },

        // ---- 缩略图缓存 ----
        { L"缩略图缓存", profile + L"\\AppData\\Local\\Microsoft\\Windows\\Explorer", true },

        // ---- DNS 缓存 (不删文件，刷新即可) 略过 ----

        // ---- 日志文件 ----
        { L"Windows 日志", L"C:\\Windows\\Logs", true },
    };

    std::wcout << L"\n开始分析清理目标...\n";
    std::wcout << L"共 " << targets.size() << L" 个目标待扫描。\n";

    ExecuteClean(targets);

    // 打印汇总
    std::wcout << L"\n";
    std::wcout << L"╔════════════════════════════════════════╗\n";
    std::wcout << L"║           清理完成 - 汇总报告           ║\n";
    std::wcout << L"╠════════════════════════════════════════╣\n";
    std::wcout << L"║  已清理空间 : " << std::setw(14) << FormatSize(g_totalDeleted) << L"  ║\n";
    std::wcout << L"║  失败项     : " << std::setw(14) << g_totalFailed << L"  ║\n";
    std::wcout << L"╚════════════════════════════════════════╝\n";

    std::wcout << L"\n按任意键退出...";
    std::wcin.get();
    return 0;
}
