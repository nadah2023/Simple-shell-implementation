#include <gtk/gtk.h>
#include "shell.h"

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Shell GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window {"
        "   background-color: #2c3e50;"
        "}"
        "label {"
        "   color: #ecf0f1;"
        "   font-size: 18px;"
        "}"
        "entry {"
        "   background-color: #34495e;"
        "   color: #ecf0f1;"
        "   border: 1px solid #1abc9c;"
        "   padding: 4px;"
        "   font-size: 18px;"
        "}"
        "button {"
        "   background-color: #1abc9c;"
        "   color: #ecf0f1;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 18px;"
        "}"
        "button:hover {"
        "   background-color: #16a085;"
        "}"
        "textview {"
        "   background-color: #34495e;"
        "   color: #ecf0f1;"
        "   border: 1px solid #1abc9c;"
        "   padding: 4px;"
        "   font-size: 18px;"
        "}"
        , -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget *label = gtk_label_new("Welcome to Shell GUI");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

    GtkWidget *button = gtk_button_new_with_label("Run Shell Command");
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    // Apply the same CSS class to text view content
    GtkStyleContext *context = gtk_widget_get_style_context(text_view);
    gtk_style_context_add_class(context, "textview");

    g_object_set_data(G_OBJECT(window), "entry", entry);
    g_object_set_data(G_OBJECT(window), "buffer", buffer);
    g_signal_connect(button, "clicked", G_CALLBACK(run_shell_command), window);
    g_signal_connect(entry, "activate", G_CALLBACK(run_shell_command), window); // Connect the "activate" signal of the entry

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.ShellGUI", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

