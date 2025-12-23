#ifndef WSL_CORE_H
#define WSL_CORE_H

#include <wchar.h>

// Common WSL distro information
typedef struct {
    wchar_t name[128];
    wchar_t state[64];
    int version;
    int is_default;
} Distro;

typedef struct {
    wchar_t name[128];
    wchar_t friendly_name[256];
} OnlineDistro;

// Function to run command and get output as a wide string
wchar_t* get_wsl_output(const wchar_t* command);

// WSL Operations
int run_wsl_action(const wchar_t* action, const wchar_t* name);
int install_wsl_distro(const wchar_t* name);
int get_online_distros(OnlineDistro* distros, int max_count);

#endif // WSL_CORE_H
