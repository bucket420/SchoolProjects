#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define HIST_SIZE 50
#define CMD_SIZE 512
#define MAX_ARGS 512

// Environment variables
extern char **environ;

// Split `str` into multiple substrings using delimiter `delim`
// Store the result in `substrings` and return the number of substrings
int split(char *substrings[], char *str, const char *delim) {
  char *token = strtok(str, delim);
  int i = 0;
  while (token != NULL) {
    substrings[i] = token;
    token = strtok(NULL, delim);
    i++;
  }
  return i;
}

// Print working directory
void pwd() {
  char dir[CMD_SIZE];
  printf("%s\n", getcwd(dir, CMD_SIZE));
}

// Change directory
void cd(char *command) {
  if (chdir(command + 3) == -1) {
    perror("cd failed");
  }
}

// Execute a relative path program
void exec_rel_path(char *command) {
  char *args[MAX_ARGS] = {NULL};
  char cmd_cpy[CMD_SIZE];
  memset(cmd_cpy, '\0', CMD_SIZE);
  strncpy(cmd_cpy, command, CMD_SIZE);
  split(args, cmd_cpy, " ");

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
  } else if (pid == 0) {
    if (execve(args[0], args, environ) == -1) {
      perror("execution failed");
      exit(0);
    }
  } else {
    wait(NULL);
  }
}

// Search for an executable file in one directory
// If found, return `true` and store full path in `exe_path`
// Else, return `false`
bool scan_dir(char *exe_path, char *exe_name, char *dir_path) {
  memset(exe_path, '\0', CMD_SIZE);
  DIR *dir = opendir(dir_path);
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strncmp(exe_name, entry->d_name, strlen(entry->d_name)) == 0) {
      strncpy(exe_path, dir_path, strlen(dir_path));
      strcat(exe_path, "/");
      strcat(exe_path, exe_name);
      if (access(exe_path, X_OK) == 0) {
        return true;
      } else {
        memset(exe_path, '\0', strlen(exe_name));
      }
    }
  }
  return false;
}

// Search for an executable file in multiple of directories
// If found, return `true` and store full path in `exe_path`
// Else, return `false`
bool scan_dirs(char *exe_path, char *exe_name, char *dir_paths[]) {
  int i = 0;
  while (dir_paths[i] != NULL) {
    if (scan_dir(exe_path, exe_name, dir_paths[i])) {
      return true;
    }
    i++;
  }
  return false;
}

// Execute an arbitrary program
void exec_any(char *command, char *env_paths[]) {
  char *args[MAX_ARGS] = {NULL};
  char cmd_cpy[CMD_SIZE];
  memset(cmd_cpy, '\0', CMD_SIZE);
  strncpy(cmd_cpy, command, CMD_SIZE);
  split(args, cmd_cpy, " ");
  char path_to_exe[CMD_SIZE];
  if (!scan_dirs(path_to_exe, args[0], env_paths)) {
    printf("command not found\n");
    return;
  }
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
  } else if (pid == 0) {
    if (execve(path_to_exe, args, environ) == -1) {
      perror("execution failed");
      exit(0);
    }
  } else {
    wait(NULL);
  }
}

// Circular buffer structure to store command history
typedef struct history {
  char commands[HIST_SIZE][CMD_SIZE];
  int curr;
  int num_entries;
} history;

// Add one entry to the command history
void put(history *hist, char *item) {
  hist->curr++;
  hist->curr %= HIST_SIZE;
  memset(hist->commands[hist->curr], '\0', CMD_SIZE);
  strncpy(hist->commands[hist->curr], item, CMD_SIZE);
}

// Print command history
void print_hist(history *hist) {
  printf("Command history (most recent first):\n");
  int i = 0;
  while (i < HIST_SIZE &&
         hist->commands[(hist->curr - i + HIST_SIZE) % HIST_SIZE][0] != 0) {
    printf("%d. %s\n", i,
           hist->commands[(hist->curr - i + HIST_SIZE) % HIST_SIZE]);
    i++;
  }
}

// Get nth command from history and store the result in `command`
// If fail, return `false`. Else, return `true`.
bool get_nth_cmd(char *command, history *hist) {
  int n = atoi(command + 1);
  if (n >= hist->num_entries) {
    printf("history size exceeded\n");
    return false;
  } else {
    memset(command, '\0', CMD_SIZE);
    strncpy(command, hist->commands[(hist->curr - n + HIST_SIZE) % HIST_SIZE],
            CMD_SIZE);
    return true;
  }
}

// Main function
int main(int argc, char **argv) {
  char env_path[CMD_SIZE];
  memset(env_path, '\0', CMD_SIZE);
  strncpy(env_path, getenv("PATH"), CMD_SIZE);

  char *paths[MAX_ARGS] = {NULL};
  int num_paths = split(paths, env_path, ":");

  printf("Welcome to MyShell!\nPath: ");
  for (int j = 0; j < num_paths; j++) {
    printf("\t%s\n", paths[j]);
  }

  history hist;
  hist.curr = 0;
  hist.num_entries = 0;

  char command[CMD_SIZE];
  while (true) {
    printf("$ ");
    memset(command, '\0', CMD_SIZE);
    fgets(command, CMD_SIZE, stdin);
    command[strcspn(command, "\n")] = 0; // Remove newline character
  execute:
    if (strncmp(command, "exit", 4) == 0) {
      break;
    } else if (strncmp(command, "pwd", 3) == 0) {
      pwd();
    } else if (strncmp(command, "cd ", 3) == 0) {
      cd(command);
    } else if (strncmp(command, "history", 7) == 0) {
      print_hist(&hist);
      continue;
    } else if (strncmp(command, "./", 2) == 0) {
      exec_rel_path(command);
    } else if (*command == '!') {
      if (!get_nth_cmd(command, &hist))
        continue;
      printf("%s\n", command);
      goto execute;
    } else {
      exec_any(command, paths);
    }
    put(&hist, command);
    if (hist.num_entries < HIST_SIZE)
      hist.num_entries++;
  }
  return 0;
}
