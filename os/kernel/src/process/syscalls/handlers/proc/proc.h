#pragma once

void _exit(int status);
int _execve(const char *pathname, char *const argv[], char *const envp[]);
int _getpid();
void* _sbrk(int increment);
