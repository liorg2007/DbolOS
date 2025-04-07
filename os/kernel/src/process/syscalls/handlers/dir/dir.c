#include "dir.h"
#include <fcntl.h>
#include "memory/heap/heap.h"
#include "process/manager/process_manager.h"
#include "filesystem/fat/fat.h"
#include "process/syscalls/handlers/file/file.h"

int _chdir(const char *pathname)
{
    process_t* current_process = get_current_process();
    char newPath[256];

    if (pathname[0] == '/')
    {
        strncpy(newPath, pathname, sizeof(newPath));
    }
    else
    {
        join_path(current_process->cwd, pathname, newPath, sizeof(newPath));
    }

    // char* normalized = simplify_path(newPath);

    FileData tmp_dir;
    if (fat_get_dir_data(newPath, &tmp_dir) != 0)
    {
        return -ENOENT;
    }

    if (tmp_dir.file_entry.attr & FAT_ATTR_DIRECTORY)
    {
        strncpy(current_process->cwd, newPath, sizeof(current_process->cwd));
        return 0;
    }
    else
    {
        return -ENOTDIR;
    }
}

int _mkdir(const char *pathname, mode_t mode)
{
    int code;
    process_t* current_process = get_current_process();
    char newPath[256];

    if (pathname[0] == '/')
    {
        strncpy(newPath, pathname, sizeof(newPath));
    }
    else
    {
        join_path(current_process->cwd, pathname, newPath, sizeof(newPath));
    }

    if((code = fat_create_directory(newPath)) != 0)
    {
        return code;
    }

    return 0;
}

int _rmdir(const char *pathname)
{
    int code;
    process_t* current_process = get_current_process();
    char newPath[256];

    if (pathname[0] == '/')
    {
        strncpy(newPath, pathname, sizeof(newPath));
    }
    else
    {
        join_path(current_process->cwd, pathname, newPath, sizeof(newPath));
    }

    if (get_glob_fd(newPath) != NULL)
    {
        return -EBUSY;
    }

    if((code = fat_delete_dir(newPath)) != 0)
    {
        return code;
    }

    return 0;
}

int _rename(const char *oldpath, const char *newpath)
{
    process_t* current_process = get_current_process();
    char fullOldPath[256];
    char fullNewPath[256];

    if (oldpath[0] == '/')
    {
        strncpy(fullOldPath, oldpath, sizeof(fullOldPath));
    }
    else
    {
        join_path(current_process->cwd, oldpath, fullOldPath, sizeof(fullOldPath));
    }

    join_path(current_process->cwd, newpath, fullNewPath, sizeof(fullNewPath));

    char* oldFormattedPath = simplify_path(fullOldPath);
    char* newFormattedPath = simplify_path(fullNewPath);

    char newName[12];
    char oldDir[256];
    char newDir[256];
    get_parent_dir(oldFormattedPath, oldDir, sizeof(oldDir));
    get_parent_dir(newFormattedPath, newDir, sizeof(newDir));

    if (strcmp(oldDir, newDir) != 0)
    {
        return -EINVAL;
    }

    get_base_name_with_len(newFormattedPath, newName, 11);
    
    if (fat_rename(oldFormattedPath, newName) != 0)
    {
        return -EINVAL;
    }

    global_file_descriptor* file_fd = get_opened_fd(oldFormattedPath);
    if (file_fd != NULL)
    {
        strncpy(file_fd->path, newFormattedPath, sizeof(file_fd->path));    
    }

    return 0;
}

int _unlink(const char *path)
{
    int r;
    process_t *current_process = get_current_process();
    char full_path[256] = {0};

    if (path[0] == '/')
    {
        strncpy(full_path, path, sizeof(full_path));
    }
    else
    {
        join_path(current_process->cwd, path, full_path, sizeof(full_path));
    }

    FileData tmp = {0};
    if((r = fat_get_file_data(full_path, &tmp)) != 0)
    {
        return r;
    }

    if (get_opened_fd(full_path) == NULL)
    {    
        r = fat_delete_file(full_path);
    }
    else
    {
        r = -EBUSY;
    }

    return r;
}

int _getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count)
{
    int r;
    int buff_index = 0;
    process_t *current_process = get_current_process();
    FileData dir;
    FAT16_DirEntry entry;
    
    if (fd < 0 || fd >= MAX_LOCAL_FD)
        return -EBADF;
    if (!current_process->fd_table[fd].is_used)
        return -EBADF;
    if (!(current_process->fd_table[fd].flags & O_DIRECTORY))
        return -ENOTDIR;
    if (current_process->fd_table[fd].flags & O_RDONLY)
        return -EPERM;

    if (current_process->fd_table[fd].offset >= current_process->fd_table[fd].global_fd->file.file_entry.file_size)
        return 0;
    
    // get dir entries
    while (count > 0)
    {
        if (current_process->fd_table[fd].offset >= current_process->fd_table[fd].global_fd->file.file_entry.file_size)
            return buff_index;

        if ((r = fat_get_dir_entry(&current_process->fd_table[fd].global_fd->file.file_entry, current_process->fd_table[fd].offset, &entry)) != 0)
        {    
            if (r == FILE_NOT_FOUND)
            {
                break;
            }
            return r;
        }
        int name_len = strlen(entry.name);
        int entry_size = sizeof(struct linux_dirent) + name_len + 1;
        if (entry_size > count)
            break;

        struct linux_dirent* tmp = kmalloc(entry_size);
        if (!tmp) return -ENOMEM;

        tmp->d_ino = entry.start_cluster;
        tmp->d_reclen = entry_size;
        tmp->d_off = current_process->fd_table[fd].offset;
        strcpy(tmp->d_name, entry.name);
        tmp->d_name[name_len] = 0;
        tmp->d_name[name_len + 1] = (entry.attr & FAT_ATTR_DIRECTORY) ? DT_DIR : DT_REG;
        memcpy((void*)((int)dirp + buff_index), tmp, entry_size);
        kfree(tmp);

        current_process->fd_table[fd].offset++;
        buff_index += entry_size;
        count -= entry_size;
    }

    return buff_index;
}

int _getcwd(char *buf, unsigned long size)
{
    process_t *current_process = get_current_process();
    if (strlen(current_process->cwd) + 1 > size)
        return -ERANGE;
    strcpy(buf, current_process->cwd);
    return 0;
}

void join_path(const char* cwd, const char* pathname, char* full_path, size_t size) {
    size_t cwd_len = strlen(cwd);
    size_t path_len = strlen(pathname);
    size_t total_len = cwd_len + path_len + 2; // +2 for '/' and null terminator
    
    if (total_len > size) {
        full_path[0] = '\0';
        return;
    }

    memset(full_path, 0, size);
    
    if (pathname[0] == '/') {
        strncpy(full_path, pathname, path_len + 1);
        return;
    }
    
    strncpy(full_path, cwd, cwd_len);
    if (full_path[cwd_len - 1] != '/')
    {
        full_path[cwd_len] = '/';
    }
    else
    {
        cwd_len--;
    }
    strncpy(full_path + cwd_len + 1, pathname, path_len + 1);
    total_len = cwd_len + path_len + 1;
    if (full_path[total_len - 1] == '.')
    {
        if (total_len - 2 >= 0 && full_path[total_len - 2] == '/')
        {
            full_path[total_len - 2] = '\0';
            full_path[total_len - 1] = '\0';
        }
    }
    if (full_path[0] == '\0')
    {
        full_path[0] = '/';
    }
}

char *simplify_path(const char *path) {
    if (path == NULL) return NULL;

    int is_absolute = (path[0] == '/');
    size_t len = strlen(path);

    // Split the path into components
    char **components = NULL;
    size_t components_size = 0;
    size_t components_capacity = 0;

    size_t start = 0;
    for (size_t i = 0; i <= len; i++) {
        if (path[i] == '/' || path[i] == '\0') {
            size_t end = i - 1;
            if (start <= end) {
                size_t comp_len = end - start + 1;
                char *comp = kmalloc(comp_len + 1);
                if (!comp) {
                    // Free existing components and return NULL
                    for (size_t j = 0; j < components_size; j++) {
                        kfree(components[j]);
                    }
                    kfree(components);
                    return NULL;
                }
                strncpy(comp, path + start, comp_len);
                comp[comp_len] = '\0';

                // Add to components array (without realloc)
                if (components_size >= components_capacity) {
                    size_t new_capacity = components_capacity == 0 ? 4 : components_capacity * 2;
                    char **new_components = kmalloc(new_capacity * sizeof(char *));
                    if (!new_components) {
                        kfree(comp);
                        for (size_t j = 0; j < components_size; j++) {
                            kfree(components[j]);
                        }
                        kfree(components);
                        return NULL;
                    }
                    // Copy existing components
                    for (size_t j = 0; j < components_size; j++) {
                        new_components[j] = components[j];
                    }
                    // Free old array
                    kfree(components);
                    components = new_components;
                    components_capacity = new_capacity;
                }
                components[components_size++] = comp;
            }
            start = i + 1;
        }
    }

    // Process components into stack
    char **stack = NULL;
    size_t stack_size = 0;
    size_t stack_capacity = 0;

    for (size_t i = 0; i < components_size; i++) {
        char *comp = components[i];
        if (strcmp(comp, ".") == 0) {
            kfree(comp);
        } else if (strcmp(comp, "..") == 0) {
            if (is_absolute) {
                // Absolute path: pop if stack is not empty
                if (stack_size > 0) {
                    kfree(stack[--stack_size]);
                }
                kfree(comp);
            } else {
                // Relative path: pop only if top is not ".."
                if (stack_size > 0 && strcmp(stack[stack_size - 1], "..") != 0) {
                    kfree(stack[--stack_size]);
                    kfree(comp);
                } else {
                    // Push ".." (without realloc)
                    if (stack_size >= stack_capacity) {
                        size_t new_capacity = stack_capacity == 0 ? 4 : stack_capacity * 2;
                        char **new_stack = kmalloc(new_capacity * sizeof(char *));
                        if (!new_stack) {
                            kfree(comp);
                            goto error;
                        }
                        // Copy existing stack
                        for (size_t j = 0; j < stack_size; j++) {
                            new_stack[j] = stack[j];
                        }
                        // Free old stack
                        kfree(stack);
                        stack = new_stack;
                        stack_capacity = new_capacity;
                    }
                    stack[stack_size++] = comp;
                }
            }
        } else {
            // Push regular component (without realloc)
            if (stack_size >= stack_capacity) {
                size_t new_capacity = stack_capacity == 0 ? 4 : stack_capacity * 2;
                char **new_stack = kmalloc(new_capacity * sizeof(char *));
                if (!new_stack) {
                    kfree(comp);
                    goto error;
                }
                // Copy existing stack
                for (size_t j = 0; j < stack_size; j++) {
                    new_stack[j] = stack[j];
                }
                // Free old stack
                kfree(stack);
                stack = new_stack;
                stack_capacity = new_capacity;
            }
            stack[stack_size++] = comp;
        }
    }

    kfree(components); // Free components array, elements are either freed or in stack

    // Build the result
    size_t total_len = 0;
    if (is_absolute) {
        if (stack_size == 0) {
            total_len = 1; // "/"
        } else {
            total_len = 1; // Leading '/'
            for (size_t i = 0; i < stack_size; i++) {
                total_len += strlen(stack[i]);
            }
            total_len += stack_size - 1; // Slashes between components
        }
    } else {
        if (stack_size == 0) {
            total_len = 1; // "."
        } else {
            for (size_t i = 0; i < stack_size; i++) {
                total_len += strlen(stack[i]);
            }
            total_len += stack_size - 1; // Slashes between components
        }
    }

    char *result = kmalloc(total_len + 1); // +1 for null terminator
    if (!result) {
        goto error;
    }

    size_t pos = 0;
    if (is_absolute) {
        result[pos++] = '/';
        if (stack_size > 0) {
            strcpy(result + pos, stack[0]);
            pos += strlen(stack[0]);
            for (size_t i = 1; i < stack_size; i++) {
                result[pos++] = '/';
                strcpy(result + pos, stack[i]);
                pos += strlen(stack[i]);
            }
        }
    } else {
        if (stack_size == 0) {
            result[pos++] = '.';
        } else {
            strcpy(result + pos, stack[0]);
            pos += strlen(stack[0]);
            for (size_t i = 1; i < stack_size; i++) {
                result[pos++] = '/';
                strcpy(result + pos, stack[i]);
                pos += strlen(stack[i]);
            }
        }
    }
    result[pos] = '\0';

    // Free stack elements and stack array
    for (size_t i = 0; i < stack_size; i++) {
        kfree(stack[i]);
    }
    kfree(stack);

    return result;

error:
    // Cleanup in case of error
    if (components) {
        for (size_t j = 0; j < components_size; j++) {
            kfree(components[j]);
        }
        kfree(components);
    }
    if (stack) {
        for (size_t j = 0; j < stack_size; j++) {
            kfree(stack[j]);
        }
        kfree(stack);
    }
    return NULL;
}

void get_parent_dir(const char *path, char *parent_dir, size_t size)
{
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));

    char *last_slash = strrchr(path_copy, '/');
    if (last_slash) {
        *(last_slash) = '\0';
    }

    char *directory = path_copy;
    
    if (strlen(directory) == 0)
    {
        strncpy(parent_dir, "/", size);
        return;
    }

    strncpy(parent_dir, directory, size);
}

void get_base_name(const char *path, char *name, size_t size)
{
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));

    char *last_slash = strrchr(path_copy, '/');
    if (last_slash) {
        *last_slash = '\0';
    }

    char *base_name = last_slash ? last_slash + 1 : path_copy;

    strncpy(name, base_name, size);
}

void get_base_name_with_len(const char *path, char *name, int name_len)
{
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));

    char *last_slash = strrchr(path_copy, '/');
    if (last_slash) {
        *last_slash = '\0';
    }

    char *base_name = last_slash ? last_slash + 1 : path_copy;
    if (strlen(base_name) > name_len)
    {
        strncpy(name, base_name, name_len);
        name[name_len] = '\0'; // Ensure null-termination
        return;
    }
    strncpy(name, base_name, name_len);
}
