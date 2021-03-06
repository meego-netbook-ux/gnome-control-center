/*
 * Copyright (c) 2010 Intel, Inc.
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

#include "shell-search-renderer.h"
#include <string.h>

G_DEFINE_TYPE (ShellSearchRenderer, shell_search_renderer, GTK_TYPE_CELL_RENDERER)

#define SEARCH_RENDERER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SHELL_TYPE_SEARCH_RENDERER, ShellSearchRendererPrivate))

struct _ShellSearchRendererPrivate
{
  gchar *title;
  gchar *search_target;
  gchar *search_string;

  PangoLayout *layout;
};

enum
{
  PROP_TITLE = 1,
  PROP_SEARCH_TARGET,
  PROP_SEARCH_STRING
};


static void
shell_search_renderer_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  switch (property_id)
    {
  case PROP_TITLE:
  case PROP_SEARCH_TARGET:
  case PROP_SEARCH_STRING:
    break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
shell_search_renderer_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ShellSearchRendererPrivate *priv = SHELL_SEARCH_RENDERER (object)->priv;

  switch (property_id)
    {
  case PROP_TITLE:
    g_free (priv->title);
    priv->title = g_value_dup_string (value);
    break;

  case PROP_SEARCH_TARGET:
    g_free (priv->search_target);
    priv->search_target = g_value_dup_string (value);
    break;

  case PROP_SEARCH_STRING:
    g_free (priv->search_string);
    priv->search_string = g_value_dup_string (value);
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
shell_search_renderer_dispose (GObject *object)
{
  ShellSearchRendererPrivate *priv = SHELL_SEARCH_RENDERER (object)->priv;

  if (priv->layout)
    {
      g_object_unref (priv->layout);
      priv->layout = NULL;
    }

  G_OBJECT_CLASS (shell_search_renderer_parent_class)->dispose (object);
}

static void
shell_search_renderer_finalize (GObject *object)
{
  ShellSearchRendererPrivate *priv = SHELL_SEARCH_RENDERER (object)->priv;

  if (priv->title)
    {
      g_free (priv->title);
      priv->title = NULL;
    }

  if (priv->search_target)
    {
      g_free (priv->search_target);
      priv->search_target = NULL;
    }

  if (priv->search_string)
    {
      g_free (priv->search_string);
      priv->search_string = NULL;
    }

  G_OBJECT_CLASS (shell_search_renderer_parent_class)->finalize (object);
}

static void
shell_search_renderer_set_layout (ShellSearchRenderer *cell, GtkWidget *widget)
{
  gchar *display_string;
  ShellSearchRendererPrivate *priv = cell->priv;
  gchar *needle, *haystack;
  gchar *full_string;

  if (!priv->layout)
    {
      priv->layout = pango_layout_new (gtk_widget_get_pango_context (widget));
      pango_layout_set_ellipsize (priv->layout, PANGO_ELLIPSIZE_END);
    }

  full_string = priv->search_target;

  needle = g_utf8_casefold (priv->search_string, -1);
  haystack = g_utf8_casefold (full_string, -1);

  /* clear any previous attributes */
  pango_layout_set_attributes (priv->layout, NULL);

  if (priv->search_string && priv->title
      && (strstr (haystack, needle)))
    {
      gchar *start;
      gchar *lead, *trail, *leaddot;
      gint count;
#define CONTEXT 10

      count = strlen (needle);
      start = strstr (haystack, needle);

      lead = MAX (start - CONTEXT, haystack);
      trail = start + count;

      if (lead == haystack)
        leaddot = "";
      else
        leaddot = "...";

      lead = g_strndup (lead, start - lead);

      display_string = g_markup_printf_escaped ("%s\n"
                                                "<small>%s%s<b>%s</b>%s</small>",
                                                priv->title, leaddot, lead,
                                                needle, trail);

      g_free (lead);
    }
  else
    display_string = g_markup_escape_text (priv->title, -1);


  pango_layout_set_markup (priv->layout, display_string, -1);
  g_free (display_string);
  g_free (needle);
  g_free (haystack);
}

static void
shell_search_renderer_get_size (GtkCellRenderer    *cell,
                                GtkWidget          *widget,
                                GdkRectangle       *cell_area,
                                gint               *x_offset,
                                gint               *y_offset,
                                gint               *width,
                                gint               *height)
{
  ShellSearchRendererPrivate *priv = SHELL_SEARCH_RENDERER (cell)->priv;
  PangoRectangle rect;

  shell_search_renderer_set_layout (SHELL_SEARCH_RENDERER (cell), widget);

  pango_layout_set_width (priv->layout, PANGO_SCALE * 180);
  pango_layout_get_pixel_extents (priv->layout, NULL, &rect);

  if (width) *width = rect.width;
  if (height) *height = rect.height;

  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;

}

static void
shell_search_renderer_render (GtkCellRenderer      *cell,
                              GdkDrawable          *window,
                              GtkWidget            *widget,
                              GdkRectangle         *background_area,
                              GdkRectangle         *cell_area,
                              GdkRectangle         *expose_area,
                              GtkCellRendererState  flags)
{
  ShellSearchRendererPrivate *priv = SHELL_SEARCH_RENDERER (cell)->priv;
  cairo_t *cr;
  PangoRectangle rect;

  shell_search_renderer_set_layout (SHELL_SEARCH_RENDERER (cell), widget);

  cr = gdk_cairo_create (window);

  pango_layout_get_pixel_extents (priv->layout, NULL, &rect);

  cairo_move_to (cr, cell_area->x, cell_area->y);

  /* FIXME: get the correct colour from the theme */
  cairo_set_source_rgb (cr, 0, 0, 0);
  if (priv->layout)
    pango_cairo_layout_path (cr, priv->layout);
  cairo_fill (cr);

  cairo_destroy (cr);
}

static void
shell_search_renderer_class_init (ShellSearchRendererClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_renderer = GTK_CELL_RENDERER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ShellSearchRendererPrivate));

  object_class->get_property = shell_search_renderer_get_property;
  object_class->set_property = shell_search_renderer_set_property;
  object_class->dispose = shell_search_renderer_dispose;
  object_class->finalize = shell_search_renderer_finalize;

  cell_renderer->get_size = shell_search_renderer_get_size;
  cell_renderer->render = shell_search_renderer_render;

  pspec = g_param_spec_string ("title",
                               "Title",
                               "Item title",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TITLE, pspec);

  pspec = g_param_spec_string ("search-target",
                               "Search Target",
                               "The string that will be searched",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SEARCH_TARGET, pspec);

  pspec = g_param_spec_string ("search-string",
                               "Search String",
                               "Current search string",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SEARCH_STRING, pspec);
}

static void
shell_search_renderer_init (ShellSearchRenderer *self)
{
  self->priv = SEARCH_RENDERER_PRIVATE (self);
}

ShellSearchRenderer *
shell_search_renderer_new (void)
{
  return g_object_new (SHELL_TYPE_SEARCH_RENDERER, NULL);
}
