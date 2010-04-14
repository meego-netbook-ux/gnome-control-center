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

#include <config.h>

#include "cc-shell-category-view.h"
#include "cc-shell-item-view.h"
#include "cc-shell.h"
#include "control-center.h"
#include "cc-shell-model.h"

G_DEFINE_TYPE (CcShellCategoryView, cc_shell_category_view, GTK_TYPE_FRAME)

#define SHELL_CATEGORY_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_SHELL_CATEGORY_VIEW, CcShellCategoryViewPrivate))

enum
{
  PROP_NAME = 1,
  PROP_MODEL
};

struct _CcShellCategoryViewPrivate
{
  gchar *name;
  GtkTreeModel *model;

  GtkWidget *header;
  GtkWidget *iconview;
};

static void
cc_shell_category_view_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CcShellCategoryViewPrivate *priv = CC_SHELL_CATEGORY_VIEW (object)->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_shell_category_view_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CcShellCategoryViewPrivate *priv = CC_SHELL_CATEGORY_VIEW (object)->priv;

  switch (property_id)
    {
    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;

    case PROP_MODEL:
      priv->model = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_shell_category_view_dispose (GObject *object)
{
  CcShellCategoryViewPrivate *priv = CC_SHELL_CATEGORY_VIEW (object)->priv;

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  G_OBJECT_CLASS (cc_shell_category_view_parent_class)->dispose (object);
}

static void
cc_shell_category_view_finalize (GObject *object)
{
  CcShellCategoryViewPrivate *priv = CC_SHELL_CATEGORY_VIEW (object)->priv;

  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }

  G_OBJECT_CLASS (cc_shell_category_view_parent_class)->finalize (object);
}

static void
cc_shell_category_view_constructed (GObject *object)
{
  CcShellCategoryViewPrivate *priv = CC_SHELL_CATEGORY_VIEW (object)->priv;
  gchar *header_name;
  GtkWidget *iconview, *vbox, *header, *self;

  self = GTK_WIDGET (object);

  iconview = cc_shell_item_view_new ();
  gtk_icon_view_set_model (GTK_ICON_VIEW (iconview), priv->model);

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (iconview), COL_PIXBUF);
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (iconview), COL_NAME);
#if HAVE_MOBLIN
    {
      GList *renderers, *l;

      gtk_icon_view_set_orientation (GTK_ICON_VIEW (iconview),
                                     GTK_ORIENTATION_HORIZONTAL);
      gtk_icon_view_set_item_width (GTK_ICON_VIEW (iconview), 200);
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

  /* create the header if required */
  if (priv->name)
    {
      header_name = g_strdup_printf ("<b>%s</b>", priv->name);

      header = g_object_new (GTK_TYPE_LABEL,
                             "use-markup", TRUE,
                             "label", header_name,
                             "wrap", TRUE,
                             "xalign", 0.0,
                             "xpad", 12,
                             "ypad", 6,
                             NULL);

      g_free (header_name);
      gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 3);

      priv->header = header;
    }

  /* add the iconview to the vbox */
#if HAVE_MOBLIN
  gtk_box_pack_start (GTK_BOX (vbox), iconview, TRUE, TRUE, 0);
#else
  gtk_box_pack_start (GTK_BOX (vbox), iconview, FALSE, TRUE, 0);
#endif

  /* add the main vbox to the view */
  gtk_container_add (GTK_CONTAINER (object), vbox);
  gtk_widget_show_all (vbox);

  priv->iconview = iconview;
}

static void
cc_shell_category_view_style_set (GtkWidget *widget,
                                  GtkStyle  *old_style)
{
  CcShellCategoryViewPrivate *priv = CC_SHELL_CATEGORY_VIEW (widget)->priv;

#ifndef HAVE_MOBLIN
  if (priv->header)
    {
      gtk_widget_modify_bg (priv->header, GTK_STATE_NORMAL,
                            &widget->style->base[GTK_STATE_NORMAL]);
      gtk_widget_modify_fg (priv->header, GTK_STATE_NORMAL,
                            &widget->style->text[GTK_STATE_NORMAL]);
    }
#endif

  if (priv->iconview)
    {
      gtk_widget_modify_bg (priv->iconview, GTK_STATE_NORMAL,
                            &widget->style->base[GTK_STATE_NORMAL]);
      gtk_widget_modify_fg (priv->iconview, GTK_STATE_NORMAL,
                            &widget->style->text[GTK_STATE_NORMAL]);
    }
}

static void
cc_shell_category_view_class_init (CcShellCategoryViewClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcShellCategoryViewPrivate));

  object_class->get_property = cc_shell_category_view_get_property;
  object_class->set_property = cc_shell_category_view_set_property;
  object_class->dispose = cc_shell_category_view_dispose;
  object_class->finalize = cc_shell_category_view_finalize;
  object_class->constructed = cc_shell_category_view_constructed;

  widget_class->style_set = cc_shell_category_view_style_set;

  pspec = g_param_spec_string ("name",
                               "Name",
                               "Name of the category",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_object ("model",
                               "Model",
                               "Model of the category",
                               GTK_TYPE_TREE_MODEL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
                               | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);

}

static void
cc_shell_category_view_init (CcShellCategoryView *self)
{
  self->priv = SHELL_CATEGORY_VIEW_PRIVATE (self);

#if HAVE_MOBLIN
  gtk_frame_set_shadow_type (GTK_FRAME (self), GTK_SHADOW_IN);
#else
  gtk_frame_set_shadow_type (GTK_FRAME (self), GTK_SHADOW_NONE);
#endif
}

GtkWidget *
cc_shell_category_view_new (const gchar  *name,
                            GtkTreeModel *model)
{
  return g_object_new (CC_TYPE_SHELL_CATEGORY_VIEW,
                       "name", name,
                       "model", model, NULL);
}

CcShellItemView*
cc_shell_category_view_get_item_view (CcShellCategoryView *self)
{
  return (CcShellItemView*) self->priv->iconview;
}
