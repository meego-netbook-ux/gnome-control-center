/*
 * network-setup-panels
 * Copyright (C) 2010 Intel Corporation
 * Copyright (C) 2011 Collabora Ltd
 * 
 * network-setup-panels is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * network-setup-panels is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CC_NETWORK_SETUP_PANEL_H_
#define _CC_NETWORK_SETUP_PANEL_H_

#include <glib-object.h>
#include "cc-panel.h"

G_BEGIN_DECLS

#define CC_TYPE_NETWORK_SETUP_PANEL             (cc_network_setup_panel_get_type ())
#define CC_NETWORK_SETUP_PANEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CC_TYPE_NETWORK_SETUP_PANEL, CcNetworkSetupPanel))
#define CC_NETWORK_SETUP_PANEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CC_TYPE_NETWORK_SETUP_PANEL, CcNetworkSetupPanelClass))
#define CC_IS_NETWORK_SETUP_PANEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CC_TYPE_NETWORK_SETUP_PANEL))
#define CC_IS_NETWORK_SETUP_PANEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CC_TYPE_NETWORK_SETUP_PANEL))
#define CC_NETWORK_SETUP_PANEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CC_TYPE_NETWORK_SETUP_PANEL, CcNetworkSetupPanelClass))

typedef struct _CcNetworkSetupPanelClass CcNetworkSetupPanelClass;
typedef struct _CcNetworkSetupPanel CcNetworkSetupPanel;

struct _CcNetworkSetupPanelClass
{
  CcPanelClass parent_class;
};

struct _CcNetworkSetupPanel
{
  CcPanel parent_instance;

  GtkWidget *socket;
  GtkWidget *scrolled;
};

GType cc_network_setup_panel_get_type (void) G_GNUC_CONST;
void cc_network_setup_panel_register (GIOModule *module);

G_END_DECLS

#endif /* _CC_NETWORK_SETUP_PANEL_H_ */
