#include <windows.h>

#include "Test.h"
#include "ServerTestInterface.h"

ONLY_TEST("Run a client in a separate process")
    ServerTestInterface s;
    s.run();

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    CreateProcess(nullptr, "client-debug.exe -debug -auto-login -username alice -server-ip 127.0.0.1",
                nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);

    WAIT_UNTIL(s.users().size() == 1);
    
    TerminateProcess(pi.hProcess, 0);
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return true;
TEND
