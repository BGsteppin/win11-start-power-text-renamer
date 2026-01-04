// ==WindhawkMod==
// @id              win11-start-power-text-renamer
// @name            Windows 11 Start Power Text Renamer
// @description     Rename the Windows 11 Start menu power options (Lock, Sleep, Shut down, Restart) to any custom text.
// @version         1.0.2
// @author          BGsteppin
// @github          https://github.com/BGsteppin
// @include         StartMenuExperienceHost.exe
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Windows 11 Start Power Text Renamer

This mod allows you to rename the **Windows 11 Start menu power options**:

- Lock
- Sleep
- Shut down
- Restart

### Features
- Custom text for each power option
- No UI styling changes (text only)
- Lightweight and safe
- Works on Windows 11 Start menu

### Notes
- Requires Windows 11
- Targets `StartMenuExperienceHost.exe`
- Changes are applied dynamically

### License
MIT
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- logReplacements: true
  $name: "Log replacements"
  $description: "Log every successful replacement (debug)"

- matchLock: "Lock"
  $name: "Match text (Lock)"
  $description: "Exact text currently shown by Windows"

- replaceLock: "Lock"
  $name: "Replace text (Lock)"
  $description: "New text to display"

- matchSleep: "Sleep"
  $name: "Match text (Sleep)"
  $description: "Exact text currently shown by Windows"

- replaceSleep: "Sleep"
  $name: "Replace text (Sleep)"
  $description: "New text to display"

- matchShutDown: "Shut down"
  $name: "Match text (Shut down)"
  $description: "Exact text currently shown by Windows"

- replaceShutDown: "Power Off"
  $name: "Replace text (Shut down)"
  $description: "New text to display"

- matchRestart: "Restart"
  $name: "Match text (Restart)"
  $description: "Exact text currently shown by Windows"

- replaceRestart: "Restart"
  $name: "Replace text (Restart)"
  $description: "New text to display"
*/
// ==/WindhawkModSettings==

#include <windhawk_api.h>
#include <windows.h>
#include <winstring.h>

static struct {
    bool logReplacements;

    wchar_t matchLock[128];
    wchar_t replaceLock[128];

    wchar_t matchSleep[128];
    wchar_t replaceSleep[128];

    wchar_t matchShutDown[128];
    wchar_t replaceShutDown[128];

    wchar_t matchRestart[128];
    wchar_t replaceRestart[128];
} g_cfg;

static void LoadSettings() {
    g_cfg.logReplacements = Wh_GetIntSetting(L"logReplacements") != 0;

    // Wh_GetStringSetting returns an allocated string which must be freed with Wh_FreeStringSetting.
    auto copySetting = [](const wchar_t* key, wchar_t* outBuf, size_t outCch, const wchar_t* fallback) {
        PCWSTR v = Wh_GetStringSetting(key); // allocated by Windhawk
        bool shouldFree = (v != nullptr);

        if (!v || !*v) {
            // If v is null or empty, use fallback.
            // Still free v if it was non-null (empty string).
            v = fallback;
            shouldFree = (v != fallback) && shouldFree; // i.e. free only original allocation
        }

        wcsncpy_s(outBuf, outCch, v, _TRUNCATE);

        if (shouldFree && v != fallback) {
            // Defensive: only free Windhawk-allocated strings
            Wh_FreeStringSetting(v);
        } else if (shouldFree && v == fallback) {
            // v was replaced with fallback, but original allocation existed; we need to free original.
            // However we lost the original pointer. So handle empty/null properly by not overwriting v.
            // (This branch should never happen due to logic above.)
        }
    };

    // To avoid losing the original pointer when v is empty, do the logic explicitly:
    auto copySettingSafe = [](const wchar_t* key, wchar_t* outBuf, size_t outCch, const wchar_t* fallback) {
        PCWSTR v = Wh_GetStringSetting(key); // allocated by Windhawk (or nullptr)
        if (!v || !*v) {
            wcsncpy_s(outBuf, outCch, fallback, _TRUNCATE);
            if (v) Wh_FreeStringSetting(v);
            return;
        }
        wcsncpy_s(outBuf, outCch, v, _TRUNCATE);
        Wh_FreeStringSetting(v);
    };

    copySettingSafe(L"matchLock",        g_cfg.matchLock,        _countof(g_cfg.matchLock),        L"Lock");
    copySettingSafe(L"replaceLock",      g_cfg.replaceLock,      _countof(g_cfg.replaceLock),      L"Lock");

    copySettingSafe(L"matchSleep",       g_cfg.matchSleep,       _countof(g_cfg.matchSleep),       L"Sleep");
    copySettingSafe(L"replaceSleep",     g_cfg.replaceSleep,     _countof(g_cfg.replaceSleep),     L"Sleep");

    copySettingSafe(L"matchShutDown",    g_cfg.matchShutDown,    _countof(g_cfg.matchShutDown),    L"Shut down");
    copySettingSafe(L"replaceShutDown",  g_cfg.replaceShutDown,  _countof(g_cfg.replaceShutDown),  L"Power Off");

    copySettingSafe(L"matchRestart",     g_cfg.matchRestart,     _countof(g_cfg.matchRestart),     L"Restart");
    copySettingSafe(L"replaceRestart",   g_cfg.replaceRestart,   _countof(g_cfg.replaceRestart),   L"Restart");

    Wh_Log(L"PowerTextRenamer: Settings loaded.");
}

static const wchar_t* GetReplacementIfMatch(PCWSTR src) {
    if (!src) return nullptr;

    // Exact match only — avoids changing unrelated UI text.
    if (g_cfg.matchLock[0] && wcscmp(src, g_cfg.matchLock) == 0) return g_cfg.replaceLock;
    if (g_cfg.matchSleep[0] && wcscmp(src, g_cfg.matchSleep) == 0) return g_cfg.replaceSleep;
    if (g_cfg.matchShutDown[0] && wcscmp(src, g_cfg.matchShutDown) == 0) return g_cfg.replaceShutDown;
    if (g_cfg.matchRestart[0] && wcscmp(src, g_cfg.matchRestart) == 0) return g_cfg.replaceRestart;

    return nullptr;
}

// -----------------------------------------------------------------------------
// Hook: WindowsCreateString / WindowsCreateStringReference (combase.dll)
// -----------------------------------------------------------------------------
typedef HRESULT(WINAPI* WindowsCreateString_t)(PCNZWCH sourceString, UINT32 length, HSTRING* string);
static WindowsCreateString_t WindowsCreateString_Original = nullptr;

HRESULT WINAPI WindowsCreateString_Hook(PCNZWCH sourceString, UINT32 length, HSTRING* string) {
    if (sourceString && length > 0) {
        // sourceString may not be null-terminated, so copy to a temp buffer.
        wchar_t tmp[256];
        UINT32 copyLen = (length < _countof(tmp) - 1) ? length : (_countof(tmp) - 1);
        wcsncpy_s(tmp, _countof(tmp), sourceString, copyLen);
        tmp[copyLen] = L'\0';

        const wchar_t* repl = GetReplacementIfMatch(tmp);
        if (repl) {
            if (g_cfg.logReplacements) {
                Wh_Log(L"PowerTextRenamer: WindowsCreateString replace '%s' -> '%s'", tmp, repl);
            }
            return WindowsCreateString_Original(repl, (UINT32)wcslen(repl), string);
        }
    }

    return WindowsCreateString_Original(sourceString, length, string);
}

typedef HRESULT(WINAPI* WindowsCreateStringReference_t)(
    PCWSTR sourceString,
    UINT32 length,
    HSTRING_HEADER* hstringHeader,
    HSTRING* string
);
static WindowsCreateStringReference_t WindowsCreateStringReference_Original = nullptr;

HRESULT WINAPI WindowsCreateStringReference_Hook(
    PCWSTR sourceString,
    UINT32 length,
    HSTRING_HEADER* hstringHeader,
    HSTRING* string
) {
    if (sourceString && length > 0) {
        wchar_t tmp[256];
        UINT32 copyLen = (length < _countof(tmp) - 1) ? length : (_countof(tmp) - 1);
        wcsncpy_s(tmp, _countof(tmp), sourceString, copyLen);
        tmp[copyLen] = L'\0';

        const wchar_t* repl = GetReplacementIfMatch(tmp);
        if (repl) {
            if (g_cfg.logReplacements) {
                Wh_Log(L"PowerTextRenamer: WindowsCreateStringReference replace '%s' -> '%s'", tmp, repl);
            }

            // Keep semantics: create a reference HSTRING using the provided header.
            // repl lives in our global config buffers, so it remains valid.
            UINT32 replLen = (UINT32)wcslen(repl);
            return WindowsCreateStringReference_Original(repl, replLen, hstringHeader, string);
        }
    }

    return WindowsCreateStringReference_Original(sourceString, length, hstringHeader, string);
}

// -----------------------------------------------------------------------------
// Hook: LoadStringW (user32.dll) fallback
// -----------------------------------------------------------------------------
typedef int(WINAPI* LoadStringW_t)(HINSTANCE, UINT, LPWSTR, int);
static LoadStringW_t LoadStringW_Original = nullptr;

int WINAPI LoadStringW_Hook(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cchBufferMax) {
    int ret = LoadStringW_Original(hInstance, uID, lpBuffer, cchBufferMax);
    if (ret > 0 && lpBuffer && cchBufferMax > 0) {
        const wchar_t* repl = GetReplacementIfMatch(lpBuffer);
        if (repl) {
            if (g_cfg.logReplacements) {
                Wh_Log(L"PowerTextRenamer: LoadStringW replace '%s' -> '%s' (id=%u)", lpBuffer, repl, uID);
            }
            wcsncpy_s(lpBuffer, cchBufferMax, repl, _TRUNCATE);
            ret = (int)wcslen(lpBuffer);
        }
    }
    return ret;
}

// -----------------------------------------------------------------------------
// Windhawk entry points
// -----------------------------------------------------------------------------
BOOL Wh_ModInit() {
    Wh_Log(L"PowerTextRenamer: Init");
    LoadSettings();

    HMODULE combase = GetModuleHandleW(L"combase.dll");
    if (!combase) combase = LoadLibraryW(L"combase.dll");
    if (!combase) {
        Wh_Log(L"PowerTextRenamer: ERROR: combase.dll not loaded");
        return FALSE;
    }

    auto pWindowsCreateString = (void*)GetProcAddress(combase, "WindowsCreateString");
    auto pWindowsCreateStringReference = (void*)GetProcAddress(combase, "WindowsCreateStringReference");
    if (!pWindowsCreateString || !pWindowsCreateStringReference) {
        Wh_Log(L"PowerTextRenamer: ERROR: Missing WindowsCreateString exports");
        return FALSE;
    }

    Wh_SetFunctionHook(pWindowsCreateString,
        (void*)WindowsCreateString_Hook,
        (void**)&WindowsCreateString_Original);

    Wh_SetFunctionHook(pWindowsCreateStringReference,
        (void*)WindowsCreateStringReference_Hook,
        (void**)&WindowsCreateStringReference_Original);

    // Fallback for resource-based text
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto pLoadStringW = (void*)GetProcAddress(user32, "LoadStringW");
        if (pLoadStringW) {
            Wh_SetFunctionHook(pLoadStringW,
                (void*)LoadStringW_Hook,
                (void**)&LoadStringW_Original);
        }
    }

    Wh_Log(L"PowerTextRenamer: Hooks installed. Open Start -> Power to test.");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"PowerTextRenamer: Uninit");
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"PowerTextRenamer: Settings changed");
    LoadSettings();
}
