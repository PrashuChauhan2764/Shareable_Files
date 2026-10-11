// GrayscaleToggle.c
// Toggles full-screen grayscale on Windows using the Magnification API.
// First launch  -> screen turns grayscale and the program stays running (hidden).
// Second launch -> signals the first instance to restore color, then both exit.
//
// Compile (MinGW-w64, no console window):
//   x86_64-w64-mingw32-gcc -O2 -mwindows -o GrayscaleToggle.exe GrayscaleToggle.c

#include <windows.h>

typedef struct tagMAGCOLOREFFECT {
    float transform[5][5];
} MAGCOLOREFFECT;

typedef BOOL (WINAPI *MagInitialize_t)(void);
typedef BOOL (WINAPI *MagUninitialize_t)(void);
typedef BOOL (WINAPI *MagSetFullscreenColorEffect_t)(const MAGCOLOREFFECT*);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    HANDLE hMutex = CreateMutexA(NULL, TRUE, "Local\\GrayscaleToggleMutex_v1");
    HANDLE hEvent = CreateEventA(NULL, TRUE, FALSE, "Local\\GrayscaleToggleEvent_v1");
    DWORD mutexStatus = GetLastError();

    // Already running -> tell the first instance to turn grayscale off
    if (mutexStatus == ERROR_ALREADY_EXISTS) {
        SetEvent(hEvent);
        CloseHandle(hEvent);
        CloseHandle(hMutex);
        return 0;
    }

    HMODULE hMag = LoadLibraryA("Magnification.dll");
    if (!hMag) {
        MessageBoxA(NULL, "Could not load Magnification.dll. This feature requires Windows 8 or later.", "Grayscale Toggle", MB_ICONERROR);
        CloseHandle(hEvent);
        CloseHandle(hMutex);
        return 1;
    }

    MagInitialize_t MagInitialize = (MagInitialize_t)GetProcAddress(hMag, "MagInitialize");
    MagUninitialize_t MagUninitialize = (MagUninitialize_t)GetProcAddress(hMag, "MagUninitialize");
    MagSetFullscreenColorEffect_t MagSetFullscreenColorEffect =
        (MagSetFullscreenColorEffect_t)GetProcAddress(hMag, "MagSetFullscreenColorEffect");

    if (!MagInitialize || !MagUninitialize || !MagSetFullscreenColorEffect) {
        MessageBoxA(NULL, "This Windows version doesn't support the required display feature.", "Grayscale Toggle", MB_ICONERROR);
        FreeLibrary(hMag);
        CloseHandle(hEvent);
        CloseHandle(hMutex);
        return 1;
    }

    if (!MagInitialize()) {
        FreeLibrary(hMag);
        CloseHandle(hEvent);
        CloseHandle(hMutex);
        return 1;
    }

    // Luminance-weighted grayscale matrix
    MAGCOLOREFFECT grayscale = { {
        { 0.30f, 0.30f, 0.30f, 0.0f, 0.0f },
        { 0.59f, 0.59f, 0.59f, 0.0f, 0.0f },
        { 0.11f, 0.11f, 0.11f, 0.0f, 0.0f },
        { 0.00f, 0.00f, 0.00f, 1.0f, 0.0f },
        { 0.00f, 0.00f, 0.00f, 0.0f, 1.0f }
    } };

    MagSetFullscreenColorEffect(&grayscale);

    // Stay alive (effect only lasts while this process runs) until a second launch signals us
    WaitForSingleObject(hEvent, INFINITE);

    // Restore normal colors (identity matrix)
    MAGCOLOREFFECT identity = { {
        { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }
    } };
    MagSetFullscreenColorEffect(&identity);
    MagUninitialize();
    FreeLibrary(hMag);

    CloseHandle(hEvent);
    CloseHandle(hMutex);
    return 0;
}
