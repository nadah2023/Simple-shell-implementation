#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <gtk/gtk.h>
#include "shell.h"

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define HISTORY_SIZE 10

char *history[HISTORY_SIZE];
int history_count = 0;

const char *known_commands[] = {
    "cd", "pwd", "history", "ls", "echo", "grep", "sort",
    "mkdir", "rmdir", "touch", "cp", "mv", "cat",
    "head", "tail", "chmod", "chown", "ps", "kill", "top",
    "du", "df", "find", "wc", "uniq", "tar", "gzip", "ping",
    "ifconfig", "date", "uptime", "help", "calculate"
};

void add_to_history(char *command) {
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(command);
    } else {
        free(history[0]);
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            history[i] = history[i + 1];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
}

void show_history(GtkTextBuffer *buffer) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    for (int i = 0; i < history_count; i++) {
        gtk_text_buffer_insert(buffer, &iter, history[i], -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    }
}

void cd(char **args, GtkTextBuffer *buffer) {
    if (args[1] == NULL) {
        gtk_text_buffer_insert_at_cursor(buffer, "cd: expected argument to \"cd\"\n", -1);
    } else if (chdir(args[1]) != 0) {
        gtk_text_buffer_insert_at_cursor(buffer, "cd: error changing directory\n", -1);
    }
}

void pwd(GtkTextBuffer *buffer) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        gtk_text_buffer_insert_at_cursor(buffer, cwd, -1);
        gtk_text_buffer_insert_at_cursor(buffer, "\n", -1);
    } else {
        gtk_text_buffer_insert_at_cursor(buffer, "pwd: error retrieving current directory\n", -1);
    }
}

void date_cmd(GtkTextBuffer *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date_str[256];
    snprintf(date_str, sizeof(date_str), "Current date and time: %02d-%02d-%02d %02d:%02d:%02d\n",
             tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    gtk_text_buffer_insert_at_cursor(buffer, date_str, -1);
}

void uptime_cmd(GtkTextBuffer *buffer) {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        long hours = info.uptime / 3600;
        long minutes = (info.uptime % 3600) / 60;
        long seconds = info.uptime % 60;
        char uptime_str[256];
        snprintf(uptime_str, sizeof(uptime_str), "Uptime: %ld hours, %ld minutes, %ld seconds\n", hours, minutes, seconds);
        gtk_text_buffer_insert_at_cursor(buffer, uptime_str, -1);
    } else {
        gtk_text_buffer_insert_at_cursor(buffer, "uptime: error retrieving uptime\n", -1);
    }
}

void help_cmd(GtkTextBuffer *buffer) {
    gtk_text_buffer_insert_at_cursor(buffer, "List of available commands:\n", -1);
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    for (int i = 0; i < sizeof(known_commands) / sizeof(known_commands[0]); ++i) {
        gtk_text_buffer_insert(buffer, &iter, known_commands[i], -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    }
}

void calculate(char **args, GtkTextBuffer *buffer) {
    char buffer_str[MAX_COMMAND_LENGTH];
    snprintf(buffer_str, sizeof(buffer_str), "echo '%s' | bc", args[1]);
    FILE *pipe = popen(buffer_str, "r");
    if (pipe == NULL) {
        gtk_text_buffer_insert_at_cursor(buffer, "popen error\n", -1);
        return;
    }

    char result[MAX_COMMAND_LENGTH];
    if (fgets(result, sizeof(result), pipe) != NULL) {
        gtk_text_buffer_insert_at_cursor(buffer, "Result: ", -1);
        gtk_text_buffer_insert_at_cursor(buffer, result, -1);
    } else {
        gtk_text_buffer_insert_at_cursor(buffer, "fgets error\n", -1);
    }

    pclose(pipe);
}

int levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int matrix[len1 + 1][len2 + 1];

    for (int i = 0; i <= len1; i++) {
        for (int j = 0; j <= len2; j++) {
            if (i == 0) {
                matrix[i][j] = j;
            } else if (j == 0) {
                matrix[i][j] = i;
            } else if (s1[i - 1] == s2[j - 1]) {
                matrix[i][j] = matrix[i - 1][j - 1];
            } else {
                matrix[i][j] = 1 + fmin(matrix[i - 1][j - 1], fmin(matrix[i - 1][j], matrix[i][j - 1]));
            }
        }
    }
    return matrix[len1][len2];
}

void recommend_command(const char *command, GtkTextBuffer *buffer) {
    int min_distance = INT_MAX;
    const char *best_match = NULL;

    for (int i = 0; i < sizeof(known_commands) / sizeof(known_commands[0]); ++i) {
        int distance = levenshtein_distance(command, known_commands[i]);
        if (distance < min_distance) {
            min_distance = distance;
            best_match = known_commands[i];
        }
    }

    // Check if any similar command is found within a reasonable distance
    if (best_match && min_distance <= 3) {
        char recommendation[MAX_COMMAND_LENGTH];
        snprintf(recommendation, sizeof(recommendation), "Command not found. Did you mean \"%s\"?\n", best_match);
        gtk_text_buffer_insert_at_cursor(buffer, recommendation, -1);
    } else {
        gtk_text_buffer_insert_at_cursor(buffer, "Command not found.\nTry one of these commands: ", -1);
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);
        for (int i = 0; i < sizeof(known_commands) / sizeof(known_commands[0]); ++i) {
            gtk_text_buffer_insert(buffer, &iter, known_commands[i], -1);
            gtk_text_buffer_insert(buffer, &iter, " ", -1);
        }
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    }
}

void execute_command(char **args, GtkTextBuffer *buffer) {
    if (args[0] == NULL) {
        return; // Empty command
    }

    if (strcmp(args[0], "cd") == 0) {
        cd(args, buffer);
        return;
    } else if (strcmp(args[0], "pwd") == 0) {
        pwd(buffer);
        return;
    } else if (strcmp(args[0], "history") == 0) {
        show_history(buffer);
        return;
    } else if (strcmp(args[0], "date") == 0) {
        date_cmd(buffer);
        return;
    } else if (strcmp(args[0], "uptime") == 0) {
        uptime_cmd(buffer);
        return;
    } else if (strcmp(args[0], "help") == 0) {
        help_cmd(buffer);
        return;
    } else if (strcmp(args[0], "calculate") == 0) {
        if (args[1] != NULL) {
            calculate(args, buffer);
        } else {
            gtk_text_buffer_insert_at_cursor(buffer, "Please provide an expression to calculate.\n", -1);
        }
        return;
    }

    // Check if the command is a known command
    int is_known_command = 0;
    for (int i = 0; i < sizeof(known_commands) / sizeof(known_commands[0]); ++i) {
        if (strcmp(args[0], known_commands[i]) == 0) {
            is_known_command = 1;
            break;
        }
    }

    if (!is_known_command) {
        recommend_command(args[0], buffer);
        return;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        gtk_text_buffer_insert_at_cursor(buffer, "pipe error\n", -1);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) { // Child process
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        close(pipe_fd[1]);
        execvp(args[0], args); // Execute command
        perror("execvp error");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        gtk_text_buffer_insert_at_cursor(buffer, "fork error\n", -1);
    } else { // Parent process
        close(pipe_fd[1]);
        char output[1024];
        ssize_t count;
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);
        while ((count = read(pipe_fd[0], output, sizeof(output) - 1)) > 0) {
            output[count] = '\0';
            gtk_text_buffer_insert(buffer, &iter, output, -1);
        }
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);
    }
}

void run_shell_command(GtkWidget *widget, gpointer data) {
    GtkWidget *entry = g_object_get_data(G_OBJECT(data), "entry");
    GtkTextBuffer *buffer = g_object_get_data(G_OBJECT(data), "buffer");
    const char *command = gtk_entry_get_text(GTK_ENTRY(entry));
    add_to_history((char *)command);

    char *args[MAX_ARGS];
    int argc = 0;

    char *command_copy = strdup(command);
    char *token = strtok(command_copy, " ");
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

    execute_command(args, buffer);

    free(command_copy);
    gtk_entry_set_text(GTK_ENTRY(entry), ""); // Clear entry after command execution
}

