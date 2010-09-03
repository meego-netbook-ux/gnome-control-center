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

#ifndef _CONTROL_CENTER_H
#define _CONTROL_CENTER_H

#include <gtk/gtk.h>

#include "cc-shell.h"

G_BEGIN_DECLS

#define CONTROL_TYPE_CENTER control_center_get_type()

#define CONTROL_CENTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CONTROL_TYPE_CENTER, ControlCenter))

#define CONTROL_CENTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CONTROL_TYPE_CENTER, ControlCenterClass))

#define CONTROL_IS_CENTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CONTROL_TYPE_CENTER))

#define CONTROL_IS_CENTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CONTROL_TYPE_CENTER))

#define CONTROL_CENTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CONTROL_TYPE_CENTER, ControlCenterClass))

enum
{
  OVERVIEW_PAGE,
  SEARCH_PAGE,
  CAPPLET_PAGE
};

typedef struct _ControlCenter ControlCenter;
typedef struct _ControlCenterClass ControlCenterClass;
typedef struct _ControlCenterPrivate ControlCenterPrivate;

struct _ControlCenter
{
  CcShell  parent;

  ControlCenterPrivate *priv;
};

struct _ControlCenterClass
{
  CcShellClass parent_class;
};

GType control_center_get_type (void) G_GNUC_CONST;

CcShell *control_center_new (void);
void control_center_present (ControlCenter *shell);

G_END_DECLS

#endif /* _CONTROL_CENTER_H */
