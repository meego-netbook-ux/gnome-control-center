/*
 * Copyright (c) 2009, 2010 Intel, Inc.
 * Copyright (c) 2010 Red Hat, Inc.
 *
 * The Control Center is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * The Control Center is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the Control Center; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Thomas Wood <thos@gnome.org>
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>

#include "cc-panel.h"
#include "cc-shell.h"
#include "shell-search-renderer.h"

#include <unique/unique.h>

#define W(b,x) GTK_WIDGET (gtk_builder_get_object (b, x))

enum
{
  CC_SHELL_RAISE_COMMAND = 1
};

typedef struct
{
  GtkBuilder *builder;
  GtkWidget  *notebook;
  GtkWidget  *window;
  GtkWidget  *search_entry;

  GSList *icon_views;

  gchar  *current_title;

  GtkListStore *store;
  GtkTreeModel *search_filter;
  GtkCellRenderer *search_renderer;
  gchar *filter_string;

  GHashTable *panels;

  guint32 last_time;

  gboolean ignore_release;

} ShellData;

enum
{
  COL_NAME,
  COL_DESKTOP_FILE,
  COL_ID,
  COL_PIXBUF,
  COL_CATEGORY,
  COL_SEARCH_TARGET,

  N_COLS
};

static void item_activated_cb (GtkIconView *icon_view, GtkTreePath *path, ShellData *data);

static gboolean
button_press_cb (GtkWidget *view,
                 GdkEventButton *event,
                 ShellData *data)
{
  /* ignore releases after double or tripple clicks */
  data->ignore_release = (event->type != GDK_BUTTON_PRESS);

  return FALSE;
}

static gboolean
button_release_cb (GtkWidget      *view,
                   GdkEventButton *event,
                   ShellData      *data)
{
  if (event->button == 1 && !data->ignore_release)
    {
      GList *selection;

      selection = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (view));

      if (!selection)
        return FALSE;

      data->last_time = event->time;

      item_activated_cb (GTK_ICON_VIEW (view), selection->data, data);

      g_list_free (selection);
      return TRUE;
    }

  return FALSE;
}

static void
selection_changed_cb (GtkIconView *view,
                      ShellData   *data)
{
  GSList *iconviews, *l;
  GList *selection;

  /* don't clear other selections if this icon view does not have one */
  selection = gtk_icon_view_get_selected_items (view);
  if (!selection)
    return;
  else
    g_list_free (selection);

  iconviews = data->icon_views;

  for (l = iconviews; l; l = l->next)
    {
      GtkIconView *iconview = l->data;

      if (iconview != view)
        {
          if ((selection = gtk_icon_view_get_selected_items (iconview)))
            {
              gtk_icon_view_unselect_all (iconview);
              g_list_free (selection);
            }
        }
    }
}

static gboolean
model_filter_func (GtkTreeModel *model,
                   GtkTreeIter  *iter,
                   ShellData    *data)
{
  gchar *name, *target;
  gchar *needle, *haystack;
  gboolean result;

  gtk_tree_model_get (model, iter, COL_NAME, &name,
                      COL_SEARCH_TARGET, &target, -1);

  if (!data->filter_string || !name || !target)
    {
      g_free (name);
      g_free (target);
      return FALSE;
    }

  needle = g_utf8_casefold (data->filter_string, -1);
  haystack = g_utf8_casefold (target, -1);

  result = (strstr (haystack, needle) != NULL);

  g_free (name);
  g_free (target);
  g_free (haystack);
  g_free (needle);

  return result;
}

static gboolean
category_filter_func (GtkTreeModel *model,
                      GtkTreeIter  *iter,
                      gchar        *filter)
{
  gchar *category;
  gboolean result;

  gtk_tree_model_get (model, iter, COL_CATEGORY, &category, -1);

  result = (g_strcmp0 (category, filter) == 0);

  g_free (category);

  return result;
}

static void
fill_model (ShellData *data)
{
  GSList *list, *l;
  GMenuTreeDirectory *d;
  GMenuTree *tree;
  GtkWidget *vbox, *w;

  vbox = W (data->builder, "main-vbox");

  tree = gmenu_tree_lookup (MENUDIR "/gnomecc.menu", 0);

  if (!tree)
    {
      g_warning ("Could not find control center menu");
      return;
    }

  d = gmenu_tree_get_root_directory (tree);

  list = gmenu_tree_directory_get_contents (d);

  data->store = gtk_list_store_new (N_COLS, G_TYPE_STRING, G_TYPE_STRING,
                                    G_TYPE_STRING, GDK_TYPE_PIXBUF,
                                    G_TYPE_STRING, G_TYPE_STRING);

  data->search_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (data->store), NULL);

  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (data->search_filter),
                                          (GtkTreeModelFilterVisibleFunc)
                                            model_filter_func,
                                          data, NULL);

  w = (GtkWidget *) gtk_builder_get_object (data->builder, "search-view");
  gtk_icon_view_set_model (GTK_ICON_VIEW (w), GTK_TREE_MODEL (data->search_filter));
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (w), COL_PIXBUF);


  data->search_renderer = (GtkCellRenderer*) shell_search_renderer_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), data->search_renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (w), data->search_renderer,
                                 "title", COL_NAME);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (w), data->search_renderer,
                                 "search-target", COL_SEARCH_TARGET);

  g_signal_connect (w, "button-press-event",
                    G_CALLBACK (button_press_cb), data);
  g_signal_connect (w, "button-release-event",
                    G_CALLBACK (button_release_cb), data);
  g_signal_connect (w, "selection-changed",
                    G_CALLBACK (selection_changed_cb), data);


  for (l = list; l; l = l->next)
    {
      GMenuTreeItemType type;
      type = gmenu_tree_item_get_type (l->data);
      if (type == GMENU_TREE_ITEM_DIRECTORY)
        {
          GtkTreeModel *filter;
          GtkWidget *header, *iconview;
          GSList *foo, *f;
          const gchar *dir_name;
          gchar *header_name;

          foo = gmenu_tree_directory_get_contents (l->data);
          dir_name = gmenu_tree_directory_get_name (l->data);

          filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (data->store), NULL);
          gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
                                                  (GtkTreeModelFilterVisibleFunc) category_filter_func,
                                                  g_strdup (dir_name), g_free);

          iconview = gtk_icon_view_new_with_model (GTK_TREE_MODEL (filter));

          gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (iconview), COL_PIXBUF);
          gtk_icon_view_set_text_column (GTK_ICON_VIEW (iconview), COL_NAME);

#if HAVE_MOBLIN
            {
              GList *renderers, *l;

              gtk_icon_view_set_orientation (GTK_ICON_VIEW (iconview),
                                             GTK_ORIENTATION_HORIZONTAL);
              gtk_icon_view_set_item_width (GTK_ICON_VIEW (iconview), 150);
              gtk_icon_view_set_spacing (GTK_ICON_VIEW (iconview), 6);

              /* set cell renderer yalign */
              renderers = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (iconview));
              for (l = renderers; l; l = g_list_next (l))
                {
                  g_object_set (l->data, "yalign", 0.0, "xalign", 0.0, "ypad", 0, NULL);
                }
              g_list_free (renderers);
            }
#else
          gtk_icon_view_set_item_width (GTK_ICON_VIEW (iconview), 120);
#endif

          g_signal_connect (iconview, "button-press-event",
                            G_CALLBACK (button_press_cb), data);
          g_signal_connect (iconview, "button-release-event",
                            G_CALLBACK (button_release_cb), data);
          g_signal_connect (iconview, "selection-changed",
                            G_CALLBACK (selection_changed_cb), data);

          data->icon_views = g_slist_prepend (data->icon_views, iconview);

          header_name = g_strdup_printf ("<big><b>%s</b></big>", dir_name);

          header = g_object_new (GTK_TYPE_LABEL,
                                 "use-markup", TRUE,
                                 "label", header_name,
                                 "wrap", TRUE,
                                 "xalign", 0.0,
                                 "xpad", 6,
                                 NULL);

          gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 3);
          gtk_box_pack_start (GTK_BOX (vbox), iconview, FALSE, TRUE, 0);


          for (f = foo; f; f = f->next)
            {
              if (gmenu_tree_item_get_type (f->data)
                  == GMENU_TREE_ITEM_ENTRY)
                {
                  GError *err = NULL;
                  gchar *search_target;
                  const gchar *icon = gmenu_tree_entry_get_icon (f->data);
                  const gchar *name = gmenu_tree_entry_get_name (f->data);
                  const gchar *id = gmenu_tree_entry_get_desktop_file_id (f->data);
                  const gchar *desktop = gmenu_tree_entry_get_desktop_file_path (f->data);
                  const gchar *comment = gmenu_tree_entry_get_comment (f->data);
                  GdkPixbuf *pixbuf = NULL;
                  char *icon2 = NULL;

                  if (icon != NULL && *icon == '/')
                    {
                      pixbuf = gdk_pixbuf_new_from_file_at_scale (icon, 32, 32, TRUE, &err);
                    }
                  else
                    {
                      if (icon2 == NULL && icon != NULL && g_str_has_suffix (icon, ".png"))
                        icon2 = g_strndup (icon, strlen (icon) - strlen (".png"));

                      pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                         icon2 ? icon2 : icon, 32,
                                                         GTK_ICON_LOOKUP_FORCE_SIZE,
                                                         &err);
                    }

                  if (err)
                    {
                      g_warning ("Could not load icon '%s': %s", icon2 ? icon2 : icon,
                                       err->message);
                      g_error_free (err);
                    }

                  g_free (icon2);

                  search_target = g_strconcat (name, " - ", comment, NULL);

                  gtk_list_store_insert_with_values (data->store, NULL, 0,
                                                     COL_NAME, name,
                                                     COL_DESKTOP_FILE, desktop,
                                                     COL_ID, id,
                                                     COL_PIXBUF, pixbuf,
                                                     COL_CATEGORY, dir_name,
                                                     COL_SEARCH_TARGET, search_target,
                                                     -1);

                  g_free (search_target);
                }
            }
        }
    }

}

static void
activate_panel (const gchar *id,
                const gchar *desktop_file,
                ShellData   *data)
{
  if (!cc_shell_set_panel (CC_SHELL (data->builder), id))
    {
      /* start app directly */
      g_debug ("Panel module not found for %s", id);

      GAppInfo *appinfo;
      GError *err = NULL;
      GdkAppLaunchContext *ctx;
      GKeyFile *key_file;

      key_file = g_key_file_new ();
      g_key_file_load_from_file (key_file, desktop_file, 0, &err);

      if (err)
        {
          g_warning ("Error starting \"%s\": %s", id, err->message);

          g_error_free (err);
          err = NULL;
          return;
        }

      appinfo = (GAppInfo*) g_desktop_app_info_new_from_keyfile (key_file);

      g_key_file_free (key_file);


      ctx = gdk_app_launch_context_new ();
      gdk_app_launch_context_set_screen (ctx, gdk_screen_get_default ());
      gdk_app_launch_context_set_timestamp (ctx, data->last_time);

      g_app_info_launch (appinfo, NULL, G_APP_LAUNCH_CONTEXT (ctx), &err);

      g_object_unref (appinfo);
      g_object_unref (ctx);

      if (err)
        {
          g_warning ("Error starting \"%s\": %s", id, err->message);
          g_error_free (err);
          err = NULL;
        }
    }

}

static void
item_activated_cb (GtkIconView *icon_view,
                   GtkTreePath *path,
                   ShellData   *data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name, *desktop_file, *id;

  model = gtk_icon_view_get_model (icon_view);

  /* get the iter and ensure it is valid */
  if (!gtk_tree_model_get_iter (model, &iter, path))
    return;

  gtk_tree_model_get (model, &iter, COL_NAME, &name,
                      COL_DESKTOP_FILE, &desktop_file,
                      COL_ID, &id, -1);

  g_debug ("activated id: '%s'", id);

  cc_shell_set_title (CC_SHELL (data->builder), name);

  activate_panel (id, desktop_file, data);

  g_free (id);
  g_free (desktop_file);
}

static void
shell_show_overview_page (ShellData *data)
{
  gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), OVERVIEW_PAGE);

  gtk_notebook_remove_page (GTK_NOTEBOOK (data->notebook), CAPPLET_PAGE);

  cc_shell_set_panel (CC_SHELL (data->builder), NULL);

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (data->builder, "label-title")), "");
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (data->builder, "title-alignment")));

  /* clear the search text */
  g_free (data->filter_string);
  data->filter_string = g_strdup ("");
  gtk_entry_set_text (GTK_ENTRY (data->search_entry), "");
}

static void
home_button_clicked_cb (GtkButton *button,
                        ShellData *data)
{
  shell_show_overview_page (data);
}

static void
search_entry_changed_cb (GtkEntry  *entry,
                         ShellData *data)
{

  /* if the entry text was set manually (not by the user) */
  if (!g_strcmp0 (data->filter_string, gtk_entry_get_text (entry)))
    return;

  g_free (data->filter_string);
  data->filter_string = g_strdup (gtk_entry_get_text (entry));

  g_object_set (data->search_renderer,
                "search-string", data->filter_string,
                NULL);

  if (!g_strcmp0 (data->filter_string, ""))
    {
      shell_show_overview_page (data);
    }
  else
    {
      gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (data->search_filter));
      gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), SEARCH_PAGE);

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (data->builder, "label-title")), "");
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (data->builder, "title-alignment")));
    }
}

static gboolean
search_entry_key_press_event_cb (GtkEntry    *entry,
                                 GdkEventKey *event,
                                 ShellData   *data)
{
  if (event->keyval == GDK_Return)
    {
      GtkTreePath *path;

      path = gtk_tree_path_new_first ();

      data->last_time = event->time;

      item_activated_cb ((GtkIconView *) gtk_builder_get_object (data->builder,
                                                                 "search-view"),
                         path, data);
      gtk_tree_path_free (path);
      return TRUE;
    }

  if (event->keyval == GDK_Escape)
    {
      gtk_entry_set_text (entry, "");
      return TRUE;
    }

  return FALSE;
}

static void
notebook_switch_page_cb (GtkNotebook     *book,
                         GtkNotebookPage *page,
                         gint             page_num,
                         ShellData       *data)
{
  /* make sure the home button is shown on all pages except the overview page */

  if (page_num == OVERVIEW_PAGE)
    gtk_widget_hide (W (data->builder, "home-button"));
  else
    gtk_widget_show (W (data->builder, "home-button"));

  if (page_num == CAPPLET_PAGE)
    gtk_widget_hide (W (data->builder, "search-entry"));
  else
    gtk_widget_show (W (data->builder, "search-entry"));
}

static void
search_entry_clear_cb (GtkEntry *entry)
{
  gtk_entry_set_text (entry, "");
}

static UniqueResponse
message_received (UniqueApp         *app,
                  gint               command,
                  UniqueMessageData *message_data,
                  guint              time_,
                  GtkWindow         *window)
{
  gtk_window_present (window);

  return GTK_RESPONSE_OK;
}

int
main (int argc, char **argv)
{
  ShellData *data;
  GtkWidget *widget;
  UniqueApp *unique;

  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  unique = unique_app_new_with_commands ("org.gnome.ControlCenter",
                                         NULL,
                                         "raise",
                                         CC_SHELL_RAISE_COMMAND,
                                         NULL);

  if (unique_app_is_running (unique))
    {
      unique_app_send_message (unique, 1, NULL);
      return 0;
    }

  data = g_new0 (ShellData, 1);

  data->builder = (GtkBuilder*) cc_shell_new ();

  if (!data->builder)
    {
      g_critical ("Could not build interface");
      return 1;
    }


  data->window = W (data->builder, "main-window");
  g_signal_connect (data->window, "delete-event", G_CALLBACK (gtk_main_quit),
                    NULL);

  data->notebook = W (data->builder, "notebook");

  g_signal_connect (data->notebook, "switch-page",
                    G_CALLBACK (notebook_switch_page_cb), data);

  fill_model (data);


  g_signal_connect (gtk_builder_get_object (data->builder, "home-button"),
                    "clicked", G_CALLBACK (home_button_clicked_cb), data);

  widget = (GtkWidget*) gtk_builder_get_object (data->builder, "search-entry");
  data->search_entry = widget;

  g_signal_connect (widget, "changed", G_CALLBACK (search_entry_changed_cb),
                    data);
  g_signal_connect (widget, "key-press-event",
                    G_CALLBACK (search_entry_key_press_event_cb), data);

  g_signal_connect (widget, "icon-release", G_CALLBACK (search_entry_clear_cb), data);


#ifdef HAVE_MOBLIN
    {
      GtkWidget *close_button = gtk_button_new ();
      gtk_widget_set_name (close_button, "moblin-close-button");
      gtk_container_add (GTK_CONTAINER (close_button),
                         gtk_image_new_from_icon_name ("window-close-hover",
                                                       GTK_ICON_SIZE_DIALOG));
      gtk_widget_set_name (GTK_WIDGET (gtk_builder_get_object (data->builder,
                                                               "toolbar1")),
                           "moblin-toolbar");
      gtk_box_pack_end (GTK_BOX (gtk_builder_get_object (data->builder, "hbox1")),
                        close_button, FALSE, TRUE, 6);

      g_signal_connect (close_button, "clicked", G_CALLBACK (gtk_main_quit),
                        NULL);

      gtk_window_maximize (GTK_WINDOW (data->window));
      gtk_window_set_decorated (GTK_WINDOW (data->window), FALSE);
    }
#endif

  gtk_widget_show_all (data->window);

  g_signal_connect (unique, "message-received", G_CALLBACK (message_received),
                    data->window);


  if (argc == 2)
    {
      GtkTreeIter iter;
      gboolean iter_valid;
      gchar *start_id;
      gchar *name;

      start_id = argv[1];

      iter_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (data->store),
                                                  &iter);

      while (iter_valid)
        {
          gchar *id;

          /* find the details for this item */
          gtk_tree_model_get (GTK_TREE_MODEL (data->store), &iter,
                              COL_NAME, &name,
                              COL_ID, &id,
                              -1);
          if (id && !strcmp (id, start_id))
            {
              g_free (id);
              break;
            }
          else
            {
              g_free (id);
              g_free (name);

              name = NULL;
              id = NULL;
            }

          iter_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (data->store),
                                                 &iter);
        }

      g_free (data->current_title);
      data->current_title = name;

      activate_panel (start_id, start_id, data);
    }

  gtk_main ();

  g_free (data->filter_string);
  g_free (data->current_title);
  g_free (data);

  return 0;
}
