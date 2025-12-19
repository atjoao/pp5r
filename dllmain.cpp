#define WIN32_LEAN_AND_MEAN

#include "stdafx.h"
#include "helper.hpp"
#include "logger.cpp"

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule;
std::pair DesktopDimensions = { 0, 0 }; 

// https://www.reddit.com/r/SteamDeck/comments/yk5zxl/comment/nta5vu8/
void PatchAspectRatio() {
    Log("Patching Aspect Ratio");
    float newAspect = (float)DesktopDimensions.first / (float)DesktopDimensions.second;
    Log("Detected Resolution: %dx%d (Aspect: %f)\n", DesktopDimensions.first, DesktopDimensions.second, newAspect);

    const char originalPattern[] = { 0x39, (char)0x8E, (char)0xE3, 0x3F }; // 16:9 float value
    char newBytes[4];
    std::memcpy(newBytes, &newAspect, 4);
    if (std::memcmp(originalPattern, newBytes, 4) == 0) {
        Log("Skipping: Aspect ratio matches default (16:9).\n");
        return;
    }

    Log("Patching occurrences to: %02X %02X %02X %02X\n", 
        (unsigned char)newBytes[0], (unsigned char)newBytes[1], 
        (unsigned char)newBytes[2], (unsigned char)newBytes[3]);

    std::vector<uint8_t*> matches = Memory::PatternScanAll(baseModule, "39 8E E3 3F");
    if (matches.empty()) {
        Log("ERROR: No occurrences of aspect ratio pattern found!\n");
        return;
    }
    
    int patchCount = 0;
    for (auto& addr : matches) {
        Memory::PatchBytes(addr, newBytes, 4);
        Log("Patched aspect ratio at address: %p\n", addr);
        patchCount++;
    }

    Log("Patched in total %d locations\n", patchCount);
}

// https://github.com/Sewer56/p5rpc.modloader/blob/master/p5rpc.modloader/Patches/P5R/SkipIntro.cs
void SkipIntro(){
    Log("Searching for intro skip pattern...\n");
    uint8_t* skipintroaddr = Memory::PatternScan(baseModule, "74 10 C7 ?? 0C 00 00 00");

    if (skipintroaddr){
        Log("Pattern found at address: %p\n", skipintroaddr);
        Memory::PatchBytes(skipintroaddr, "\x90\x90", 2);
        Log("Patched Skip Intro\n");
    } else {
        Log("ERROR: Intro skip pattern not found!\n");
    }
}

DWORD __stdcall Main(void*){
    #ifdef _DEBUG
        CreateDebugConsole();
    #endif
    InitLogger(thisModule, baseModule);
    SetUnhandledExceptionFilter(CrashHandler);

    SkipIntro(); 
    PatchAspectRatio();
    
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DesktopDimensions = Util::GetPhysicalDesktopDimensions();
        thisModule = hModule;
        HANDLE mainHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Main, NULL, 0, NULL);

        if (mainHandle){
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST);
            CloseHandle(mainHandle);
        }
        break; 
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}