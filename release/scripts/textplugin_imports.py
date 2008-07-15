#!BPY
"""
Name: 'Import Complete'
Blender: 246
Group: 'TextPlugin'
Shortcut: 'Space'
Tooltip: 'Lists modules when import or from is typed'
"""

# Only run if we have the required modules
OK = False
try:
	import bpy, sys
	from BPyTextPlugin import *
	OK = True
except:
	pass

def main():
	txt = bpy.data.texts.active
	line, c = current_line(txt)
	
	# Check we are in a normal context
	if get_context(line, c) != 0:
		return
	
	pos = line.rfind('from ', 0, c)
	
	# No 'from' found
	if pos == -1:
		# Check instead for straight 'import'
		pos2 = line.rfind('import ', 0, c)
		if pos2 != -1 and pos2 == c-7:
			items = [(m, 'm') for m in sys.builtin_module_names]
			items.sort(cmp = suggest_cmp)
			txt.suggest(items, '')
	
	# Immediate 'from' before cursor
	elif pos == c-5:
		items = [(m, 'm') for m in sys.builtin_module_names]
		items.sort(cmp = suggest_cmp)
		txt.suggest(items, '')
	
	# Found 'from' earlier
	else:
		pos2 = line.rfind('import ', pos+5, c)
		
		# No 'import' found after 'from' so suggest it
		if pos2 == -1:
			txt.suggest([('import', 'k')], '')
			
		# Immediate 'import' before cursor and after 'from...'
		elif pos2 == c-7 or line[c-2] == ',':
			between = line[pos+5:pos2-1].strip()
			try:
				mod = get_module(between)
			except:
				print 'Module not found:', between
				return
			
			items = [('*', 'k')]
			for (k,v) in mod.__dict__.items():
				if is_module(v): t = 'm'
				elif callable(v): t = 'f'
				else: t = 'v'
				items.append((k, t))
			items.sort(cmp = suggest_cmp)
			txt.suggest(items, '')

if OK:
	main()
