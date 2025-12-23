#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <locale.h>
#include "wsl_core.h"

#define MAX_LINE_LENGTH 1024

void list_wsl() {
    // Set locale to system default to support various characters in console
    setlocale(LC_ALL, "");

    wchar_t* output = get_wsl_output(L"wsl.exe --list --verbose");
    if (!output) {
        wprintf(L"Error: Could not execute wsl.exe or capture its output.\n");
        return;
    }

    wprintf(L"%-10s %-25s %-15s %-10s\n", L"DEFAULT", L"NAME", L"STATE", L"VERSION");
    wprintf(L"------------------------------------------------------------------\n");

    wchar_t* line;
    wchar_t* next_line;
    line = wcstok(output, L"\r\n", &next_line);
    
    int line_count = 0;
    while (line != NULL) {
        line_count++;
        if (line_count > 1) { // Skip header
            int is_default = 0;
            wchar_t name[128] = {0};
            wchar_t state[64] = {0};
            int version = 0;

            wchar_t* ptr = line;
            // Skip leading spaces
            while (*ptr == L' ') ptr++;

            if (*ptr == L'*') {
                is_default = 1;
                ptr++;
                while (*ptr == L' ') ptr++;
            }

            if (swscanf(ptr, L"%ls %ls %d", name, state, &version) >= 3) {
                wprintf(L"%-10ls %-25ls %-15ls %-10d\n", 
                        is_default ? L"*" : L"", name, state, version);
            }
        }
        line = wcstok(NULL, L"\r\n", &next_line);
    }

    free(output);
}

void list_online_wsl() {
    setlocale(LC_ALL, "");
    OnlineDistro distros[128];
    int count = get_online_distros(distros, 128);
    
    if (count == 0) {
        wprintf(L"No online distributions found or error executing wsl.exe\n");
        return;
    }

    wprintf(L"%-25s %-50s\n", L"NAME", L"FRIENDLY NAME");
    wprintf(L"--------------------------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        wprintf(L"%-25ls %-50ls\n", distros[i].name, distros[i].friendly_name);
    }
}

void run_wsl_cmd(const wchar_t* action, const wchar_t* name) {
    if (wcscmp(action, L"start") == 0) {
        wprintf(L"Starting %ls...\n", name);
    } else if (wcscmp(action, L"stop") == 0) {
        wprintf(L"Stopping %ls...\n", name);
    } else if (wcscmp(action, L"shutdown") == 0) {
        wprintf(L"Shutting down all WSL distributions...\n");
    } else if (wcscmp(action, L"unregister") == 0) {
        wprintf(L"Unregistering (Uninstalling) %ls...\n", name);
    }

    if (run_wsl_action(action, name) == 0) {
        wprintf(L"Operation successful.\n");
    } else {
        wprintf(L"Operation failed.\n");
    }
}

void install_distro(const wchar_t* name) {
    wprintf(L"Starting installation of %ls in a new window...\n", name);
    if (install_wsl_distro(name) == 0) {
        wprintf(L"Installation command sent successfully.\n");
    } else {
        wprintf(L"Failed to start installation.\n");
    }
}

int wmain(int argc, wchar_t *argv[]) {
    if (argc < 2) {
        list_wsl();
        return 0;
    }

    if (wcscmp(argv[1], L"list") == 0) {
        if (argc > 2 && wcscmp(argv[2], L"--online") == 0) {
            list_online_wsl();
        } else {
            list_wsl();
        }
    } else if (wcscmp(argv[1], L"online-list") == 0) {
        list_online_wsl();
    } else if (wcscmp(argv[1], L"start") == 0 && argc > 2) {
        run_wsl_cmd(L"start", argv[2]);
    } else if (wcscmp(argv[1], L"stop") == 0 && argc > 2) {
        run_wsl_cmd(L"stop", argv[2]);
    } else if (wcscmp(argv[1], L"unregister") == 0 && argc > 2) {
        run_wsl_cmd(L"unregister", argv[2]);
    } else if (wcscmp(argv[1], L"install") == 0 && argc > 2) {
        install_distro(argv[2]);
    } else if (wcscmp(argv[1], L"shutdown") == 0) {
        run_wsl_cmd(L"shutdown", NULL);
    } else {
        wprintf(L"Usage: %ls [command] [args]\n", argv[0]);
        wprintf(L"Commands:\n");
        wprintf(L"  list              List installed distributions\n");
        wprintf(L"  list --online     List available online distributions\n");
        wprintf(L"  online-list       Alias for list --online\n");
        wprintf(L"  start <name>      Start a distribution\n");
        wprintf(L"  stop <name>       Stop a distribution\n");
        wprintf(L"  unregister <name> Uninstall a distribution\n");
        wprintf(L"  install <name>    Install a new distribution\n");
        wprintf(L"  shutdown          Shutdown all WSL instances\n");
    }

    return 0;
}
