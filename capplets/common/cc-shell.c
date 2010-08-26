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

#include "cc-shell.h"
#include "cc-panel.h"

G_DEFINE_ABSTRACT_TYPE (CcShell, cc_shell, G_TYPE_OBJECT)

#define SHELL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_SHELL, CcShellPrivate))

static void
cc_shell_class_init (CcShellClass *klass)
{
}

static void
cc_shell_init (CcShell *self)
{
}

gboolean
cc_shell_set_panel (CcShell     *shell,
                    const gchar *id)
{
  CcShellClass *class;

  class = (CcShellClass *) G_OBJECT_GET_CLASS (shell);

  return class->set_panel (shell, id);
}

