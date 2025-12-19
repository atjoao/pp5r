#include <minwindef.h>
#include <windows.h>
#include <fstream>
std::ofstream logFile;

void Log(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    printf("%s", buffer);
    
    if (logFile.is_open()) {
        logFile << buffer;
        logFile.flush();
    }
}

void InitLogger(HMODULE thisModule, HMODULE baseModule) {
    char path[MAX_PATH];
    GetModuleFileNameA(thisModule, path, MAX_PATH);
    
    std::string logPath(path);
    size_t pos = logPath.rfind('.');
    if (pos != std::string::npos) {
        logPath = logPath.substr(0, pos) + ".log";
    } else {
        logPath += ".log";
    }
    
    logFile.open(logPath, std::ios::out | std::ios::trunc);
    Log("Base module: %p\n", baseModule);
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ex) {
    Log("Oops...");
    Log("Exception Code: 0x%08X\n", ex->ExceptionRecord->ExceptionCode);
    Log("Exception Addr: %p\n", ex->ExceptionRecord->ExceptionAddress);
    
    if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        Log("Access Violation: %s %p\n",
            ex->ExceptionRecord->ExceptionInformation[0] ? "WRITE" : "READ",
            (void*)ex->ExceptionRecord->ExceptionInformation[1]);
    }
    
    CONTEXT* ctx = ex->ContextRecord;
    Log("RIP: %016llX  RSP: %016llX\n", ctx->Rip, ctx->Rsp);
    
    if (logFile.is_open()) logFile.close();
    return EXCEPTION_CONTINUE_SEARCH;
}

void CreateDebugConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Debug Console");
    Log("Debug console initialized\n");
}