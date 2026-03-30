// clang++ -std=c++17 -O2 -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -static -static-libgcc -static-libstdc++ -o reminder.exe reminder.cpp -lshell32 -lole32 -luser32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>

#include <cmath>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <stdexcept>

// =========================
// DEFAULTS
// =========================
static const wchar_t* DEFAULT_TITLE   = L"Reminder";
static const wchar_t* DEFAULT_MESSAGE = L"Time to do the thing!";
static const DWORD    BALLOON_MS      = 5000;

// =========================
// DATA STRUCTURES
// =========================
struct Args {
    double   interval_seconds;
    std::wstring title;
    std::wstring message;
};

struct NotifierState {
    HWND            hwnd;
    NOTIFYICONDATAW nid;
    bool            icon_added;
};

// Global for cleanup handler
static NotifierState* g_state = nullptr;

// =========================
// HELPERS
// =========================
static std::wstring utf8_to_wide(const char* s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
    out.resize(len - 1); // strip embedded null from size
    return out;
}

static void safe_wcopy(wchar_t* dst, size_t dst_count, const wchar_t* src) {
    wcsncpy(dst, src, dst_count - 1);
    dst[dst_count - 1] = L'\0';
}

// =========================
// ARGUMENT PARSING
// =========================
static Args parse_args(int argc, char* argv[]) {
    Args args;
    args.title   = DEFAULT_TITLE;
    args.message = DEFAULT_MESSAGE;

    bool use_hours     = false;
    bool got_interval  = false;
    double interval    = 0.0;

    for (int i = 1; i < argc; ++i) {
        std::string tok = argv[i];
        if (tok == "-H" || tok == "--hours") {
            use_hours = true;
        } else if (tok == "-t" || tok == "--title") {
            if (i + 1 >= argc) throw std::invalid_argument("-t requires a value");
            args.title = utf8_to_wide(argv[++i]);
        } else if (tok == "-m" || tok == "--message") {
            if (i + 1 >= argc) throw std::invalid_argument("-m requires a value");
            args.message = utf8_to_wide(argv[++i]);
        } else if (!tok.empty() && tok[0] == '-') {
            throw std::invalid_argument(std::string("Unknown flag: ") + tok);
        } else {
            if (got_interval) throw std::invalid_argument("Too many positional arguments");
            try {
                interval = std::stod(tok);
            } catch (const std::exception&) {
                throw std::invalid_argument(std::string("Invalid interval: ") + tok);
            }
            if (interval <= 0.0) throw std::invalid_argument("Interval must be positive");
            got_interval = true;
        }
    }

    if (!got_interval) throw std::invalid_argument("Missing required argument: interval");

    args.interval_seconds = interval * (use_hours ? 3600.0 : 60.0);
    return args;
}

// =========================
// WINDOW + TRAY ICON
// =========================
static HWND create_message_window() {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = DefWindowProcW;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"ReminderNotifier_MsgWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"ReminderNotifier_MsgWnd", L"",
                                0, 0, 0, 0, 0,
                                HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!hwnd) throw std::runtime_error("Failed to create message window");
    return hwnd;
}

static void setup_tray_icon(NotifierState& state) {
    NOTIFYICONDATAW& nid = state.nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = state.hwnd;
    nid.uID              = 1;
    nid.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_APP + 1;
    nid.hIcon            = LoadIconW(nullptr, IDI_INFORMATION);
    safe_wcopy(nid.szTip, ARRAYSIZE(nid.szTip), L"Reminder Notifier");

    // Retry loop in case the tray isn't ready yet (e.g., at login)
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (Shell_NotifyIconW(NIM_ADD, &nid)) {
            state.icon_added = true;
            break;
        }
        Sleep(500);
    }
    if (!state.icon_added) throw std::runtime_error("Shell_NotifyIconW NIM_ADD failed");

    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

static void show_balloon(NotifierState& state, const Args& args) {
    NOTIFYICONDATAW& nid = state.nid;
    nid.uFlags      = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout    = BALLOON_MS;
    safe_wcopy(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), args.title.c_str());
    safe_wcopy(nid.szInfo,      ARRAYSIZE(nid.szInfo),      args.message.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// =========================
// CLEANUP
// =========================
static void cleanup(NotifierState& state) {
    if (state.icon_added) {
        Shell_NotifyIconW(NIM_DELETE, &state.nid);
        state.icon_added = false;
    }
    if (state.hwnd) {
        DestroyWindow(state.hwnd);
        state.hwnd = nullptr;
    }
    CoUninitialize();
}

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (g_state) cleanup(*g_state);
    return FALSE; // let the process terminate normally
}

// =========================
// TIMESTAMP
// =========================
static void print_timestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] Notification sent\n",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);
    fputs(buf, stdout);
    fflush(stdout);
}

// Pump any pending messages for the hidden window
static void pump_messages(HWND hwnd) {
    MSG msg;
    while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

// =========================
// MAIN LOOP
// =========================
static void notification_loop(NotifierState& state, const Args& args) {
    DWORD interval_ms = (args.interval_seconds >= (double)MAXDWORD / 1000.0)
                        ? MAXDWORD
                        : static_cast<DWORD>(args.interval_seconds * 1000.0);

    while (true) {
        try {
            show_balloon(state, args);
            print_timestamp();
            Sleep(BALLOON_MS);
            pump_messages(state.hwnd);
            Sleep(interval_ms);
        } catch (const std::exception& e) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Error: %s — reinitializing notifier\n", e.what());
            fputs(buf, stderr);
            if (state.icon_added) {
                Shell_NotifyIconW(NIM_DELETE, &state.nid);
                state.icon_added = false;
            }
            Sleep(1000);
            try {
                setup_tray_icon(state);
            } catch (const std::exception& reinit_err) {
                snprintf(buf, sizeof(buf), "Failed to reinitialize: %s\n", reinit_err.what());
                fputs(buf, stderr);
                Sleep(2000);
            }
        } catch (...) {
            fputs("Fatal error — reinitializing notifier\n", stderr);
            if (state.icon_added) {
                Shell_NotifyIconW(NIM_DELETE, &state.nid);
                state.icon_added = false;
            }
            Sleep(2000);
            try { setup_tray_icon(state); } catch (...) {}
        }
    }
}

// =========================
// ENTRY POINT
// =========================
int main(int argc, char* argv[]) {
    Args args;
    try {
        args = parse_args(argc, argv);
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        fprintf(stderr, "Usage: reminder <interval> [-H] [-t TITLE] [-m MESSAGE]\n");
        fprintf(stderr, "  interval   Time in minutes (or hours with -H), supports decimals\n");
        fprintf(stderr, "  -H         Treat interval as hours\n");
        fprintf(stderr, "  -t TITLE   Notification title (default: \"Reminder\")\n");
        fprintf(stderr, "  -m MSG     Notification message (default: \"Time to do the thing!\")\n");
        return 1;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    NotifierState state = {};
    state.hwnd = create_message_window();
    setup_tray_icon(state);
    g_state = &state;
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    const char* unit = (args.interval_seconds >= 3600.0 &&
                        std::fmod(args.interval_seconds, 3600.0) == 0.0)
                       ? "hours" : "minutes";
    double display_val = (std::string(unit) == "hours")
                         ? args.interval_seconds / 3600.0
                         : args.interval_seconds / 60.0;
    printf("Reminder started: every %.4g %s\n", display_val, unit);
    fflush(stdout);

    notification_loop(state, args);
    return 0;
}
