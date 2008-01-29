#!BPY

"""
Name: 'Save Render Layers...'
Blender: 245
Group: 'Render'
Tooltip: 'Save current renderlayers as a BPython script'
"""

__author__ = "Campbell Barton"
__url__ = ("blender", "elysiun")
__version__ = "1.0"

__bpydoc__ = """\
"""

# --------------------------------------------------------------------------
# The scripts generated by this script are put under Public Domain by
# default, but you are free to edit the ones you generate with this script
# and change their license to another one of your choice.
# --------------------------------------------------------------------------
# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****
# --------------------------------------------------------------------------

import Blender
from Blender.Window import Theme, FileSelector
from Blender import Scene

sce = Scene.GetCurrent()
rend = sce.render

# default filename: theme's name + '_theme.py' in user's scripts dir:
default_fname = Blender.Get("scriptsdir")
default_fname = Blender.sys.join(default_fname, sce.name + '_renderlayer.py')
default_fname = default_fname.replace(' ','_')

def write_renderlayers(filename):
	"Write the current renderlayer as a bpython script"
	
	if not filename.endswith('.py'): filename += '.py'

	fout = file(filename, "w")

	fout.write("""#!BPY

# \"\"\"
# Name: '%s'
# Blender: 245
# Group: 'Render'
# Tooltip: ''
# \"\"\"

__%s__ = "????"
__%s__ = "2.43"
__%s__ = ["blender"]
__%s__ = \"\"\"\\
You can edit this section to write something about your script that can
be read then with the Scripts Help Browser script in Blender.

Remember to also set author, version and possibly url(s) above.  You can also
define an __email__ tag, check some bundled script's source for examples.
\"\"\"

# This script was automatically generated by the save_theme.py bpython script.
# By default, these generated scripts are released as Public Domain, but you
# are free to change the license of the scripts you generate with
# save_theme.py before releasing them.

import Blender
from Blender import Scene

sce = Scene.GetCurrent()
rend = sce.render
""" % (sce.name, "author", "version", "url", "bpydoc"))

	for lay in rend.renderLayers: # 
		fout.write("\nlay = rend.addRenderLayer()\n")
		fout.write("lay.name = \"%s\"\n" % lay.name)
		
		exec("vars = dir(lay)")

		for var in vars:
			if var.startswith('_'):
				continue
				
			v = "lay.%s" % var
			exec("value = %s" % v)
			if type(value) == str:
				fout.write("%s = '%s'\n" % (v, value))
			else:
				fout.write("%s = %s\n" % (v, value))

	fout.write('\nBlender.Redraw(-1)')
	fout.close()
	try:
		Blender.UpdateMenus()
	except:
		Blender.Draw.PupMenu("Warning - check console!%t|Menus could not be automatically updated")

FileSelector(write_renderlayers, "Save RenderLayers", default_fname)