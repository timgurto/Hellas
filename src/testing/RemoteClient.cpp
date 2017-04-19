#include "RemoteClient.h"

const std::string RemoteClient::CLIENT_BINARY_PATH = "client-debug.exe";
const std::string RemoteClient::DEFAULT_ARGS = "-debug -server-ip 127.0.0.1 -auto-login";

RemoteClient::RemoteClient(const std::string &args){
    static const size_t
        STARTUP_INFO_SIZE = sizeof(_si),
        PROCESS_INFO_SIZE = sizeof(_pi);;
    ZeroMemory(&_si, STARTUP_INFO_SIZE);
    _si.cb = STARTUP_INFO_SIZE;
    ZeroMemory(&_pi, PROCESS_INFO_SIZE);

    std::string command = CLIENT_BINARY_PATH + " " + DEFAULT_ARGS + " " + args;

    CreateProcess(nullptr, const_cast<LPSTR>(command.c_str()),
            nullptr, nullptr, false, 0, nullptr, nullptr,
            &_si, &_pi);
}

RemoteClient::~RemoteClient(){
    TerminateProcess(_pi.hProcess, 0);
    CloseHandle(_pi.hProcess);
    CloseHandle(_pi.hThread);
}
