#ifndef SHELL_H
#define SHELL_H

#include <gtk/gtk.h>

void add_to_history(char *command);
void show_history(GtkTextBuffer *buffer);
void cd(char **args, GtkTextBuffer *buffer);
void pwd(GtkTextBuffer *buffer);
void date_cmd(GtkTextBuffer *buffer);
void uptime_cmd(GtkTextBuffer *buffer);
void help_cmd(GtkTextBuffer *buffer);
void calculate(char **args, GtkTextBuffer *buffer);
int levenshtein_distance(const char *s1, const char *s2);
void recommend_command(const char *command, GtkTextBuffer *buffer);
void execute_command(char **args, GtkTextBuffer *buffer);
void run_shell_command(GtkWidget *widget, gpointer data);
void activate(GtkApplication *app, gpointer user_data);

#endif // SHELL_H

