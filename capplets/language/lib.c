/*
 * lib.c
 * Copyright (C) 2010 Intel Corporation
 * 
 * language-panels is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * language-panels is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <gio/gio.h>

#include "cc-language-panel.h"

void
g_io_module_load (GIOModule *module)
{
  cc_language_panel_register (module);
}

void
g_io_module_unload (GIOModule *module)
{
}
