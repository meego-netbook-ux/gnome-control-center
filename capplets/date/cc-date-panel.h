/*
 * date-panels
 * Copyright (C) 2010 Intel Corporation
 * 
 * date-panels is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * date-panels is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CC_DATE_PANEL_H_
#define _CC_DATE_PANEL_H_

#include <glib-object.h>
#include <libgnome-control-center-extension/cc-panel.h>

G_BEGIN_DECLS

#define CC_TYPE_DATE_PANEL             (cc_date_panel_get_type ())
#define CC_DATE_PANEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CC_TYPE_DATE_PANEL, CcDatePanel))
#define CC_DATE_PANEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CC_TYPE_DATE_PANEL, CcDatePanelClass))
#define CC_IS_DATE_PANEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CC_TYPE_DATE_PANEL))
#define CC_IS_DATE_PANEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CC_TYPE_DATE_PANEL))
#define CC_DATE_PANEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CC_TYPE_DATE_PANEL, CcDatePanelClass))

typedef struct _CcDatePanelClass CcDatePanelClass;
typedef struct _CcDatePanel CcDatePanel;

struct _CcDatePanelClass
{
  CcPanelClass parent_class;
};

struct _CcDatePanel
{
  CcPanel parent_instance;

  GtkWidget *socket;
  GtkWidget *scrolled;
};

GType cc_date_panel_get_type (void) G_GNUC_CONST;
void cc_date_panel_register (GIOModule *module);

G_END_DECLS

#endif /* _CC_DATE_PANEL_H_ */
