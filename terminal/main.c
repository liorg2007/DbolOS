#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#define MAX_INPUT_LENGTH 256
#define MAX_ARGS 64
#define MAX_PATH 256
#define MAX_BUFFER_SIZE 4096

typedef int (*cmd_func)(char **args);

int execute_command(char **args);
int parse_input(char *input, char *arg_buffer, char **args);
int start_shell();
void print_cwd();

int cmd_help(char **args);
int cmd_ls(char **args);
int cmd_cd(char **args);
int cmd_cat(char **args);
int cmd_echo(char **args);
int cmd_mkdir(char **args);
int cmd_rm(char **args);
int cmd_rmdir(char **args);
int cmd_run(char **args);
int cmd_exit(char **args);

char *supported_commands[] = {
    "help",
    "ls",
    "cd",
    "cat",
    "echo",
    "mkdir",
    "rm",
    "rmdir",
    "run",
    "exit"
};

cmd_func command_funcs[] = {
    &cmd_help,
    &cmd_ls,
    &cmd_cd,
    &cmd_cat,
    &cmd_echo,
    &cmd_mkdir,
    &cmd_rm,
    &cmd_rmdir,
    &cmd_run,
    &cmd_exit
};

int num_cmds()
{
    return sizeof(supported_commands)/sizeof(char *);
}

int main() 
{
    int r = start_shell();
    return r;
}

int start_shell()
{
    char input[MAX_INPUT_LENGTH] = {0};
    // Buffer to store copies of arguments
    char arg_buffer[MAX_INPUT_LENGTH] = {0};
    char *args[MAX_ARGS] = {0};
    int status = 1;

    // Welcome message using write
    asm (
        "movl $169, %%eax\n" // Set syscall number for execve (59) into EAX
        "int $0x80\n"       // Trigger the syscall
        :
        :                   // No inputs
        : "%eax"            // EAX is clobbered
    );
    

    puts("Welcome to DBolOShell!");
    while (status)
    {
        print_cwd();
        size_t len = read(STDIN_FILENO, input, MAX_INPUT_LENGTH);
        if (len > 0) {
            input[len-1] = '\0';
        }
        else
        {
            puts("read error");
            continue;
        }
        
        if (parse_input(input, arg_buffer, args) == 0) {
            continue;  // Empty input
        }
        
        status = execute_command(args);
    }
    
    return 0;
}

void print_cwd()
{
    char cwd[MAX_PATH] = {0};
    char prompt_buffer[MAX_PATH + 20] = {0}; // Extra space for prefix

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        strcpy(cwd, "Unknown path");
    }
    printf("DBOLOS %s>", cwd);
    fflush(stdout);
}

int execute_command(char **args)
{
    if (args[0] == NULL)
    {
        return 1;
    }
    int cmd_len = strlen(args[0]);

    // check for the command
    for (int i=0; i<num_cmds(); ++i)
    {
        if (strncmp(supported_commands[i], args[0], cmd_len) == 0)
            return command_funcs[i](args);
    }

    printf("Unknown command: %s\n", args[0]);
    return 1;
}

// Custom parsing function that doesn't use strtok
int parse_input(char *input, char *arg_buffer, char **args) {
    int arg_count = 0;
    int buf_pos = 0;
    int i = 0;
    int input_len = strlen(input);
    
    // Skip leading whitespace
    while (i < input_len && isspace((unsigned char)input[i])) {
        i++;
    }
    
    while (i < input_len && arg_count < MAX_ARGS - 1) {
        // Mark start of argument
        args[arg_count] = &arg_buffer[buf_pos];
        
        // Copy characters until whitespace
        while (i < input_len && !isspace((unsigned char)input[i])) {
            arg_buffer[buf_pos++] = input[i++];
        }
        
        // Null-terminate this argument
        arg_buffer[buf_pos++] = '\0';
        arg_count++;
        
        // Skip whitespace to next argument
        while (i < input_len && isspace((unsigned char)input[i])) {
            i++;
        }
    }
    
    args[arg_count] = NULL;  // Null-terminate the args list
    return arg_count;
}

int cmd_help(char **args)
{
    const char help_msg[] = "Welcome to DBOLOS hacker terminal!!\nWe support a lot of commands:\n";
    write(STDOUT_FILENO, help_msg, sizeof(help_msg) - 1);

    for (int i=0; i<num_cmds(); ++i)
    {
        printf("-   %s\n", supported_commands[i]);
    }

    puts("Enjoy!");
    return 1;
}

int cmd_ls(char **args)
{
    DIR* dir;
    struct dirent *entry;
    
    // Default to current directory if no argument provided
    const char *path = args[1] ? args[1] : ".";
    
    if ((dir = opendir(path)) == NULL) {
        puts("ls");
        return 1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        puts(entry->d_name);
    }
    closedir(dir);

    return 1;
}

int cmd_cd(char **args)
{   
    // if no args then move to root dir '/'
    if (args[1] == NULL)
    {
        if(chdir("/"))
        {
            puts("chdir");
        }
    }
    else
    {
        if(chdir(args[1]))
        {
            puts("chdir");
        }
    }

    return 1;
}

int cmd_cat(char **args)
{
    if (args[1] == NULL)
    {
        const char err_msg[] = "cat: missing file\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return 1;
    }

    FILE* fp = fopen(args[1], "rb");

    if (!fp) {
        puts("cat");
        return 1;
    }       

    // Use a fixed-size buffer on the stack
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_read;
    
    // Read and print the file in chunks
    while ((bytes_read = fread(buffer, 1, MAX_BUFFER_SIZE, fp)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\0') {
                write(STDOUT_FILENO, "\0", 1); // Write \0 explicitly
            } else {
                write(STDOUT_FILENO, &buffer[i], 1);
            }
        }
    }
    write(STDOUT_FILENO, "\n", 1);
    
    fclose(fp);
    return 1;
}

int cmd_echo(char **args)
{
    FILE* fd = stdout;
    char buffer[MAX_INPUT_LENGTH] = {0};
    int i = 1;
    
    if (args[1] == NULL)
    {
        write(STDOUT_FILENO, "\n", 1);
        return 1;
    }
    
    // First pass: find redirection if any
    while(args[i] != NULL)
    {
        if (strcmp(args[i], ">") == 0)
        {
            if (args[i+1] == NULL)
            {
                const char err_msg[] = "echo: syntax error, expected file name\n";
                write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
                return 1;
            }

            fd = fopen(args[i+1], "w+");
            if (fd == NULL) {
                puts("echo");
                return 1;
            }
            args[i] = NULL; // Mark end of content
            break;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            if (args[i+1] == NULL)
            {
                const char err_msg[] = "echo: syntax error, expected file name\n";
                write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
                return 1;
            }

            fd = fopen(args[i+1], "a");
            if (fd == NULL) {
                puts("echo");
                return 1;
            }
            args[i] = NULL; // Mark end of content
            break;
        }
        i++;
    }
    
    // Second pass: build the output string
    buffer[0] = '\0';
    i = 1;
    while(args[i] != NULL)
    {
        if (i > 1) {
            strcat(buffer, " ");
        }
        strcat(buffer, args[i]);
        i++;
    }

    // Write to file or stdout
    if (fd == stdout) {
        write(STDOUT_FILENO, buffer, strlen(buffer));
        write(STDOUT_FILENO, "\n", 1);
    } else {
        fwrite(buffer, sizeof(char), strlen(buffer), fd);
        fclose(fd);
    }

    return 1;
}

int cmd_mkdir(char **args)
{
    if (args[1] == NULL)
    {
        const char err_msg[] = "mkdir: mkdir <name>\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return 1;
    }

    if (mkdir(args[1], 0755))
        puts("mkdir");

    return 1;
}

int cmd_rm(char **args)
{
    if (args[1] == NULL)
    {
        const char err_msg[] = "rm: rm <name>\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return 1;
    }

    if (unlink(args[1]))
        puts("rm");

    return 1;
}

int cmd_rmdir(char **args)
{
    if (args[1] == NULL)
    {
        const char err_msg[] = "rmdir: rmdir <name>\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return 1;
    }

    if (rmdir(args[1]))
        puts("rmdir");

    return 1;
}

int cmd_run(char **args)
{
    if (args[1] == NULL)
    {
        const char err_msg[] = "run: run <name>\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return 1;
    }

    const char *filename = args[1];
    char *const argv[] = {args[1], NULL};
    char *const envp[] = {NULL};
    puts("");
    int ret;
    asm volatile(
        "movl $59, %%eax\n"     // SYS_execve (11)
        "movl %1, %%ebx\n"      // filename
        "movl %2, %%ecx\n"      // argv
        "movl %3, %%edx\n"      // envp
        "int $0x80\n"           // Call kernel
        "movl %%eax, %0\n"      // Store return value in ret
        : "=r"(ret)
        : "g"(filename), "g"(argv), "g"(envp)
        : "%eax", "%ebx", "%ecx", "%edx", "memory");

    // If execve fails, it will return a negative error code.
    if (ret)
        printf("run failed %d\n", ret);
    return 1;
}

int cmd_exit(char **args)
{
    puts("You are closing the DbolOS shell!!!");
    puts("Good bye :)\n");

    asm (
        "movl $170, %%eax\n" // Set syscall number for execve (59) into EAX
        "int $0x80\n"       // Trigger the syscall
        :
        :                   // No inputs
        : "%eax"            // EAX is clobbered
    );
    

    return 0;
}