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
//  Disclaimer
// ============================================================
//  This software is provided "AS IS", without warranty of any
//  kind, express or implied. Use at your own risk.
//  The developer is NOT liable for any data loss, system
//  damage, or other losses arising from use of this software.
// ============================================================

struct CleanTarget {
    std::wstring name;
    std::wstring path;
    bool         isFolder;
};

ULONGLONG g_totalDeleted = 0;
ULONGLONG g_totalFailed  = 0;

std::wstring FormatSize(ULONGLONG bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int idx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    wchar_t buf[64];
    swprintf_s(buf, L"%.2f %s", val, units[idx]);
    return buf;
}

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
    for (auto& entry : fs::recursive_directory_iterator(folder, ec)) {
        if (ec) { ec.clear(); continue; }
        fs::remove(entry, ec);
    }
    fs::remove(folder, ec);
    return freed;
}

ULONGLONG EmptyRecycleBin() {
    ULONGLONG freed = 0;
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
    SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return freed;
}

void ExecuteClean(const std::vector<CleanTarget>& targets) {
    std::error_code ec;
    for (auto& t : targets) {
        std::wcout << L"\n[Scan] " << t.name << L"\n  Path: " << t.path << L"\n";
        if (!fs::exists(t.path, ec) && t.isFolder) {
            std::wcout << L"  -> Path not found, skipped.\n";
            continue;
        }
        if (ec) { ec.clear(); }
        ULONGLONG before = g_totalDeleted;
        if (t.name.find(L"Recycle") != std::wstring::npos) {
            g_totalDeleted += EmptyRecycleBin();
        } else {
            g_totalDeleted += DeleteFolder(t.path);
        }
        ULONGLONG diff = g_totalDeleted - before;
        std::wcout << L"  -> Freed: " << FormatSize(diff) << L"\n";
    }
}

bool ShowDisclaimer() {
    system("cls");
    std::wcout << L"\n";
    std::wcout << L"+------------------------------------------------------+\n";
    std::wcout << L"|            C Drive Junk Cleaner  v1.2                 |\n";
    std::wcout << L"+------------------------------------------------------+\n";
    std::wcout << L"|                  ** DISCLAIMER **                      |\n";
    std::wcout << L"|                                                      |\n";
    std::wcout << L"|  This software is provided \"AS IS\", without        |\n";
    std::wcout << L"|  warranty of any kind. Use at your own risk.         |\n";
    std::wcout << L"|  The developer is NOT liable for any data loss,      |\n";
    std::wcout << L"|  system damage, or other losses.                     |\n";
    std::wcout << L"|                                                      |\n";
    std::wcout << L"|  Clean scope: Temp / RecycleBin / WinUpdate Cache    |\n";
    std::wcout << L"|              Browser Cache / Prefetch / WER / Logs   |\n";
    std::wcout << L"|                                                      |\n";
    std::wcout << L"|  Close unnecessary programs before continuing!       |\n";
    std::wcout << L"+------------------------------------------------------+\n\n";
    std::wcout << L"Type YES to confirm cleanup, anything else to exit: ";
    std::wstring input;
    std::getline(std::wcin, input);
    return (input == L"YES" || input == L"yes" || input == L"Yes");
}

int wmain() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (!ShowDisclaimer()) {
        std::wcout << L"\nCleanup cancelled. Press Enter to exit...";
        std::wcin.get();
        return 0;
    }

    wchar_t userProfile[MAX_PATH];
    wchar_t localAppData[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    std::wstring profile(userProfile);
    std::wstring local(localAppData);

    std::vector<CleanTarget> targets = {
        {L"User Temp",              profile + L"\\AppData\\Local\\Temp", true},
        {L"Windows Temp",           L"C:\\Windows\\Temp", true},
        {L"Recycle Bin",            L"C:\\$Recycle.Bin", true},
        {L"Prefetch",               L"C:\\Windows\\Prefetch", true},
        {L"Windows Update Cache",   L"C:\\Windows\\SoftwareDistribution\\Download", true},
        {L"Windows Error Reports",  L"C:\\ProgramData\\Microsoft\\Windows\\WER", true},
        {L"Chrome Cache",           local + L"\\Google\\Chrome\\User Data\\Default\\Cache", true},
        {L"Chrome Code Cache",      local + L"\\Google\\Chrome\\User Data\\Default\\Code Cache", true},
        {L"Edge Cache",             local + L"\\Microsoft\\Edge\\User Data\\Default\\Cache", true},
        {L"Edge Code Cache",        local + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache", true},
        {L"Firefox Cache",          local + L"\\Mozilla\\Firefox\\Profiles", true},
        {L"Thumbnail Cache",        profile + L"\\AppData\\Local\\Microsoft\\Windows\\Explorer", true},
        {L"Windows Logs",           L"C:\\Windows\\Logs", true},
    };

    std::wcout << L"\nStarting scan... " << targets.size() << L" targets to check.\n";
    ExecuteClean(targets);

    std::wcout << L"\n";
    std::wcout << L"+--------------------------------------+\n";
    std::wcout << L"|         Cleanup Complete             |\n";
    std::wcout << L"+--------------------------------------+\n";
    std::wcout << L"|  Freed : " << std::setw(14) << FormatSize(g_totalDeleted) << L"  |\n";
    std::wcout << L"|  Failed: " << std::setw(14) << g_totalFailed << L"  |\n";
    std::wcout << L"+--------------------------------------+\n";

    std::wcout << L"\nPress Enter to exit...";
    std::wcin.get();
    return 0;
}
