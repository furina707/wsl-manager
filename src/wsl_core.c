#include "wsl_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

wchar_t* get_wsl_output(const wchar_t* command) {
    FILE* fp = _wpopen(command, L"rb");
    if (!fp) return NULL;

    unsigned char* raw_buffer = (unsigned char*)malloc(1024 * 30);
    if (!raw_buffer) {
        _pclose(fp);
        return NULL;
    }

    size_t bytes_read = fread(raw_buffer, 1, 1024 * 30, fp);
    _pclose(fp);

    if (bytes_read == 0) {
        free(raw_buffer);
        return NULL;
    }

    unsigned char* start = raw_buffer;
    if (bytes_read >= 2 && raw_buffer[0] == 0xFF && raw_buffer[1] == 0xFE) {
        start += 2;
        bytes_read -= 2;
    }

    size_t wlen = bytes_read / 2;
    wchar_t* wbuffer = (wchar_t*)malloc(sizeof(wchar_t) * (wlen + 1));
    if (!wbuffer) {
        free(raw_buffer);
        return NULL;
    }

    memcpy(wbuffer, start, wlen * 2);
    wbuffer[wlen] = L'\0';

    free(raw_buffer);
    return wbuffer;
}

int run_wsl_action(const wchar_t* action, const wchar_t* name) {
    wchar_t command[512];
    if (wcscmp(action, L"start") == 0) {
        swprintf(command, 512, L"wsl.exe -d %ls -- echo Started", name);
    } else if (wcscmp(action, L"stop") == 0) {
        swprintf(command, 512, L"wsl.exe --terminate %ls", name);
    } else if (wcscmp(action, L"shutdown") == 0) {
        swprintf(command, 512, L"wsl.exe --shutdown");
    } else if (wcscmp(action, L"unregister") == 0) {
        swprintf(command, 512, L"wsl.exe --unregister %ls", name);
    } else {
        return -1;
    }

    return _wsystem(command);
}

int install_wsl_distro(const wchar_t* name) {
    wchar_t command[512];
    swprintf(command, 512, L"start cmd /k wsl.exe --install -d %ls", name);
    return _wsystem(command);
}

int get_online_distros(OnlineDistro* distros, int max_count) {
    wchar_t* output = get_wsl_output(L"wsl.exe --list --online");
    if (!output) return 0;

    int count = 0;
    wchar_t* next_line = NULL;
    wchar_t* line = wcstok(output, L"\r\n", &next_line);
    BOOL data_started = FALSE;

    while (line != NULL && count < max_count) {
        if (!data_started) {
            if (wcsstr(line, L"NAME") && wcsstr(line, L"FRIENDLY NAME")) data_started = TRUE;
        } else {
            wchar_t* ptr = line;
            while (*ptr == L' ') ptr++;
            
            if (*ptr) {
                int i = 0;
                while (*ptr && *ptr != L' ' && i < 127) distros[count].name[i++] = *ptr++;
                distros[count].name[i] = L'\0';
                
                while (*ptr == L' ') ptr++;
                wcsncpy(distros[count].friendly_name, ptr, 255);
                distros[count].friendly_name[255] = L'\0';
                
                if (wcslen(distros[count].name) > 0) {
                    count++;
                }
            }
        }
        line = wcstok(NULL, L"\r\n", &next_line);
    }
    free(output);
    return count;
}
