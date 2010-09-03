/*
 * Copyright (c) 2010 Intel, Inc.
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

#include "control-center.h"

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>

#include "cc-panel.h"
#include "shell-search-renderer.h"
#include "cc-shell-category-view.h"
#include "cc-shell-model.h"

G_DEFINE_TYPE (ControlCenter, control_center, CC_TYPE_SHELL)

#define SHELL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CONTROL_TYPE_CENTER, ControlCenterPrivate))

struct _ControlCenterPrivate
{
  CcPanel *current_panel;
  GHashTable *panels;
  GtkBuilder *builder;



  GtkWidget  *notebook;
  GtkWidget  *window;
  GtkWidget  *search_entry;

  GtkListStore *store;

  GtkTreeModel *search_filter;
  GtkWidget *search_view;
  GtkCellRenderer *search_renderer;
  gchar *filter_string;

  gboolean ignore_release;
  guint last_time;
};

#define W(b,x) GTK_WIDGET (gtk_builder_get_object (b, x))

static void
control_center_dispose (GObject *object)
{
  G_OBJECT_CLASS (control_center_parent_class)->dispose (object);
}

static void
control_center_finalize (GObject *object)
{
  ControlCenterPrivate *priv = ((ControlCenter*) (object))->priv;

  if (priv->panels)
    {
      g_hash_table_destroy (priv->panels);
      priv->panels = NULL;
    }

  G_OBJECT_CLASS (control_center_parent_class)->finalize (object);
}

static gboolean
control_center_set_panel (CcShell     *shell,
                          const gchar *id)
{
  CcPanel *panel;
  ControlCenterPrivate *priv = CONTROL_CENTER (shell)->priv;
  GtkWidget *notebook;
  GtkWidget *title_label, *title_alignment;
  gchar *desktop_file, *name;
  GtkTreeIter iter;
  gboolean iter_valid;

  notebook = W (priv->builder, "notebook");


  if (priv->current_panel != NULL)
    cc_panel_set_active (priv->current_panel, FALSE);


  /* clear the search text */
  g_free (priv->filter_string);
  priv->filter_string = g_strdup ("");
  gtk_entry_set_text (GTK_ENTRY (priv->search_entry), "");


  title_label = W (priv->builder, "label-title");
  title_alignment = W (priv->builder, "title-alignment");


  /* if there is a current panel, remove it from the parent manually to
   * avoid it being destroyed */
  if (priv->current_panel)
    {
      GtkContainer *container;
      GtkWidget *widget;

      widget = GTK_WIDGET (priv->current_panel);
      container = (GtkContainer *) gtk_widget_get_parent (widget);
      gtk_container_remove (container, widget);

      priv->current_panel = NULL;
      gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), CAPPLET_PAGE);
    }

  /* if no id, show the overview page */
  if (id == NULL || !strcmp (id, ""))
    {
      gtk_label_set_text (GTK_LABEL (title_label), "");
      gtk_widget_hide (title_alignment);

      gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook),
                                     OVERVIEW_PAGE);
      return TRUE;
    }

  /* find the information for this id  */
  iter_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store),
                                              &iter);
  while (iter_valid)
    {
      gchar *s;

      /* find the details for this item */
      gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                          COL_ID, &s, COL_DESKTOP_FILE, &desktop_file,
                          COL_NAME, &name, -1);
      if (s && !strcmp (id, s))
        {
          g_free (s);
          break;
        }
      else
        {
          g_free (s);
          g_free (desktop_file);
          g_free (name);

          name = NULL;
          s = NULL;
          desktop_file = NULL;
        }

      iter_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store),
                                             &iter);
    }

  /* first look for a panel module */
  panel = g_hash_table_lookup (priv->panels, id);
  if (panel != NULL)
    {
      GtkWidget *scroll, *view;

      priv->current_panel = panel;
      gtk_container_set_border_width (GTK_CONTAINER (panel), 12);
      gtk_widget_show_all (GTK_WIDGET (panel));
      cc_panel_set_active (panel, TRUE);

      scroll = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
      view = gtk_viewport_new (NULL, NULL);
      gtk_viewport_set_shadow_type (GTK_VIEWPORT (view), GTK_SHADOW_NONE);
      gtk_container_add (GTK_CONTAINER (view), GTK_WIDGET (panel));

      gtk_container_add (GTK_CONTAINER (scroll), view);
      gtk_widget_show_all (scroll);

      gtk_notebook_insert_page (GTK_NOTEBOOK (notebook),
                                GTK_WIDGET (scroll), NULL, CAPPLET_PAGE);

      gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook),
                                     CAPPLET_PAGE);

      /* show title */
      gtk_label_set_text (GTK_LABEL (title_label), name);
      gtk_widget_show (title_label);
      gtk_widget_show (title_alignment);

      g_free (name);
      g_free (desktop_file);

      return TRUE;
    }
  else
    {
      GAppInfo *appinfo;
      GError *err = NULL;
      GdkAppLaunchContext *ctx;
      GKeyFile *key_file;

      /* start app directly */
      g_debug ("Panel module not found for %s", id);

      /* remove the title and show the overview page */
      gtk_label_set_text (GTK_LABEL (title_label), "");
      gtk_widget_hide (title_alignment);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), OVERVIEW_PAGE);

      if (!desktop_file)
        return FALSE;

      key_file = g_key_file_new ();
      g_key_file_load_from_file (key_file, desktop_file, 0, &err);

      g_free (name);
      g_free (desktop_file);

      if (err)
        {
          g_warning ("Error starting \"%s\": %s", id, err->message);

          g_error_free (err);
          err = NULL;
          return FALSE;
        }

      appinfo = (GAppInfo*) g_desktop_app_info_new_from_keyfile (key_file);

      g_key_file_free (key_file);


      ctx = gdk_app_launch_context_new ();
      gdk_app_launch_context_set_screen (ctx, gdk_screen_get_default ());
      gdk_app_launch_context_set_timestamp (ctx, priv->last_time);

      g_app_info_launch (appinfo, NULL, G_APP_LAUNCH_CONTEXT (ctx), &err);

      g_object_unref (appinfo);
      g_object_unref (ctx);

      if (err)
        {
          g_warning ("Error starting \"%s\": %s", id, err->message);
          g_error_free (err);
          err = NULL;
        }
      return FALSE;
    }
}

static void
control_center_class_init (ControlCenterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcShellClass *shell_class = CC_SHELL_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ControlCenterPrivate));

  object_class->dispose = control_center_dispose;
  object_class->finalize = control_center_finalize;

  shell_class->set_panel = control_center_set_panel;
}


static void
load_panel_plugins (ControlCenter *shell)
{
  ControlCenterPrivate *priv = shell->priv;
  static volatile GType panel_type = G_TYPE_INVALID;
  static GIOExtensionPoint *ep = NULL;
  GList *modules;
  GList *panel_implementations;
  GList *l;

  /* make sure base type is registered */
  if (panel_type == G_TYPE_INVALID)
    {
      panel_type = g_type_from_name ("CcPanel");
    }

  priv->panels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                        g_object_unref);

  if (ep == NULL)
    {
      ep = g_io_extension_point_register (CC_PANEL_EXTENSION_POINT_NAME);
    }

  /* load all modules */
  modules = g_io_modules_load_all_in_directory (EXTENSIONSDIR);

  /* find all extensions */
  panel_implementations = g_io_extension_point_get_extensions (ep);
  for (l = panel_implementations; l != NULL; l = l->next)
    {
      GIOExtension *extension;
      CcPanel *panel;
      char *id;

      extension = l->data;

      panel = g_object_new (g_io_extension_get_type (extension),
                            "shell", shell,
                            NULL);

      /* take ownership of the object to prevent it being destroyed */
      g_object_ref_sink (panel);

      g_object_get (panel, "id", &id, NULL);
      g_hash_table_insert (priv->panels, g_strdup (id), g_object_ref (panel));
      g_free (id);
    }

  /* unload all modules; the module our instantiated authority is in won't be
   * unloaded because we've instantiated a reference to a type in this module
   */
  g_list_foreach (modules, (GFunc) g_type_module_unuse, NULL);
  g_list_free (modules);
}


static void
item_activated_cb (CcShellCategoryView *view,
                   gchar               *name,
                   gchar               *id,
                   gchar               *desktop_file,
                   CcShell             *shell)
{
  cc_shell_set_panel (shell, id);
}


static void
search_entry_changed_cb (GtkEntry      *entry,
                         ControlCenter *shell)
{
  ControlCenterPrivate *priv = shell->priv;

  /* if the entry text was set manually (not by the user) */
  if (!g_strcmp0 (priv->filter_string, gtk_entry_get_text (entry)))
    return;

  g_free (priv->filter_string);
  priv->filter_string = g_strdup (gtk_entry_get_text (entry));

  g_object_set (priv->search_renderer,
                "search-string", priv->filter_string,
                NULL);

  if (!g_strcmp0 (priv->filter_string, ""))
    {
      cc_shell_set_panel (CC_SHELL (shell), NULL);
    }
  else
    {
      gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->search_filter));
      gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), SEARCH_PAGE);

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (priv->builder, "label-title")), "");
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (priv->builder, "title-alignment")));
    }
}

static gboolean
search_entry_key_press_event_cb (GtkEntry      *entry,
                                 GdkEventKey   *event,
                                 ControlCenter *shell)
{
  ControlCenterPrivate *priv = shell->priv;

  priv->last_time = event->time;

  if (event->keyval == GDK_Return)
    {
      GtkTreePath *path;

      path = gtk_tree_path_new_first ();

      gtk_icon_view_item_activated (GTK_ICON_VIEW (priv->search_view), path);

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
search_entry_clear_cb (GtkEntry *entry)
{
  gtk_entry_set_text (entry, "");
}


static gboolean
model_filter_func (GtkTreeModel  *model,
                   GtkTreeIter   *iter,
                   ControlCenter *shell)
{
  ControlCenterPrivate *priv = shell->priv;
  gchar *name, *target;
  gchar *needle, *haystack;
  gboolean result;

  gtk_tree_model_get (model, iter, COL_NAME, &name,
                      COL_SEARCH_TARGET, &target, -1);

  if (!priv->filter_string || !name || !target)
    {
      g_free (name);
      g_free (target);
      return FALSE;
    }

  needle = g_utf8_casefold (priv->filter_string, -1);
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
setup_search (ControlCenter *shell)
{
  GtkWidget *search_scrolled, *search_view, *widget;
  ControlCenterPrivate *priv = shell->priv;

  g_return_if_fail (priv->store != NULL);

  /* create the search filter */
  priv->search_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store),
                                                   NULL);

  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->search_filter),
                                          (GtkTreeModelFilterVisibleFunc)
                                            model_filter_func,
                                          shell, NULL);

  /* set up the search view */
  priv->search_view = search_view = cc_shell_item_view_new ();
  gtk_icon_view_set_orientation (GTK_ICON_VIEW (search_view),
                                 GTK_ORIENTATION_HORIZONTAL);
  gtk_icon_view_set_spacing (GTK_ICON_VIEW (search_view), 6);
  gtk_icon_view_set_model (GTK_ICON_VIEW (search_view),
                           GTK_TREE_MODEL (priv->search_filter));
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (search_view), COL_PIXBUF);

  search_scrolled = W (priv->builder, "search-scrolled-window");
  gtk_container_add (GTK_CONTAINER (search_scrolled), search_view);


  /* add the custom renderer */
  priv->search_renderer = (GtkCellRenderer*) shell_search_renderer_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (search_view),
                              priv->search_renderer, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (search_view),
                                 priv->search_renderer,
                                 "title", COL_NAME);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (search_view),
                                 priv->search_renderer,
                                 "search-target", COL_SEARCH_TARGET);

  /* connect the activated signal */
  g_signal_connect (search_view, "desktop-item-activated",
                    G_CALLBACK (item_activated_cb), shell);


  widget = (GtkWidget*) gtk_builder_get_object (priv->builder, "search-entry");
  priv->search_entry = widget;

  g_signal_connect (widget, "changed", G_CALLBACK (search_entry_changed_cb),
                    shell);
  g_signal_connect (widget, "key-press-event",
                    G_CALLBACK (search_entry_key_press_event_cb), shell);
  g_signal_connect (widget, "icon-release", G_CALLBACK (search_entry_clear_cb),
                    NULL);

}

static void
fill_model (ControlCenter *shell)
{
  ControlCenterPrivate *priv = shell->priv;
  GSList *list, *l;
  GMenuTreeDirectory *d;
  GMenuTree *tree;
  GtkWidget *vbox;

  vbox = W (priv->builder, "main-vbox");
  gtk_widget_set_size_request (vbox, 0, -1);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (vbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  tree = gmenu_tree_lookup (MENUDIR "/gnomecc.menu", 0);

  if (!tree)
    {
      g_warning ("Could not find control center menu");
      return;
    }

  d = gmenu_tree_get_root_directory (tree);

  list = gmenu_tree_directory_get_contents (d);

  priv->store = (GtkListStore *) cc_shell_model_new ();

  for (l = list; l; l = l->next)
    {
      GMenuTreeItemType type;
      type = gmenu_tree_item_get_type (l->data);

      if (type == GMENU_TREE_ITEM_DIRECTORY)
        {
          GtkTreeModel *filter;
          GtkWidget *categoryview;
          GSList *contents, *f;
          const gchar *dir_name;

          contents = gmenu_tree_directory_get_contents (l->data);
          dir_name = gmenu_tree_directory_get_name (l->data);

          /* create new category view for this category */
          filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store),
                                              NULL);
          gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
                                                  (GtkTreeModelFilterVisibleFunc) category_filter_func,
                                                  g_strdup (dir_name), g_free);

          categoryview = cc_shell_category_view_new (dir_name, filter);
          gtk_box_pack_start (GTK_BOX (vbox), categoryview, TRUE, TRUE, 6);
          g_signal_connect (cc_shell_category_view_get_item_view (CC_SHELL_CATEGORY_VIEW (categoryview)),
                            "desktop-item-activated",
                            G_CALLBACK (item_activated_cb), shell);
          gtk_widget_show (categoryview);

          /* add the items from this category to the model */
          for (f = contents; f; f = f->next)
            {
              if (gmenu_tree_item_get_type (f->data) == GMENU_TREE_ITEM_ENTRY)
                {
                  cc_shell_model_add_item (CC_SHELL_MODEL (priv->store),
                                           dir_name,
                                           f->data);
                }
            }
        }
    }

}

static void
home_button_clicked_cb (GtkButton     *button,
                        ControlCenter *shell)
{
  cc_shell_set_panel (CC_SHELL (shell), NULL);
}

static void
notebook_switch_page_cb (GtkNotebook     *book,
                         GtkNotebookPage *page,
                         gint             page_num,
                         ControlCenter   *shell)
{
  ControlCenterPrivate *priv = shell->priv;

  /* make sure the home button is shown on all pages except the overview page */

  if (page_num == OVERVIEW_PAGE)
    gtk_widget_hide (W (priv->builder, "home-button"));
  else
    gtk_widget_show (W (priv->builder, "home-button"));

  if (page_num == CAPPLET_PAGE)
    gtk_widget_hide (W (priv->builder, "search-entry"));
  else
    gtk_widget_show (W (priv->builder, "search-entry"));
}



static void
control_center_init (ControlCenter *self)
{
  ControlCenterPrivate *priv;
  GError *err = NULL;
  GtkWidget *close_button;

  priv = self->priv = SHELL_PRIVATE (self);


  /* load UI */
  priv->builder = gtk_builder_new ();
  gtk_builder_add_from_file (priv->builder, UIDIR "/shell.ui", &err);

  if (err)
    {
      g_warning ("Could not load UI file: %s", err->message);

      g_error_free (err);

      g_object_unref (priv->builder);
      priv->builder = NULL;

      return;
    }


  priv->window = W (priv->builder, "main-window");
  g_signal_connect (priv->window, "delete-event", G_CALLBACK (gtk_main_quit),
                    NULL);

  priv->notebook = W (priv->builder, "notebook");

  g_signal_connect (priv->notebook, "switch-page",
                    G_CALLBACK (notebook_switch_page_cb), self);

  fill_model (self);

  g_signal_connect (gtk_builder_get_object (priv->builder, "home-button"),
                    "clicked", G_CALLBACK (home_button_clicked_cb), self);

  setup_search (self);

  /* meego stuff */
  close_button = gtk_button_new ();
  gtk_widget_set_name (close_button, "moblin-close-button");
  gtk_container_add (GTK_CONTAINER (close_button),
                     gtk_image_new_from_icon_name ("window-close",
                                                   GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_widget_set_name (GTK_WIDGET (gtk_builder_get_object (priv->builder,
                                                           "toolbar1")),
                       "moblin-toolbar");
  gtk_box_pack_end (GTK_BOX (gtk_builder_get_object (priv->builder, "hbox1")),
                    close_button, FALSE, TRUE, 6);

  g_signal_connect (close_button, "clicked", G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_window_maximize (GTK_WINDOW (priv->window));
  gtk_window_set_decorated (GTK_WINDOW (priv->window), FALSE);

  /* load settings panels */
  load_panel_plugins (self);

  gtk_widget_show_all (priv->window);
}


CcShell *
control_center_new ()
{
  return g_object_new (CONTROL_TYPE_CENTER, NULL);
}

void
control_center_present (ControlCenter *shell)
{
  gtk_window_present (GTK_WINDOW (shell->priv->window));
}

