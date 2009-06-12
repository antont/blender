
import bpy

class DataButtonsPanel(bpy.types.Panel):
	__space_type__ = "BUTTONS_WINDOW"
	__region_type__ = "WINDOW"
	__context__ = "data"
	
	def poll(self, context):
		return (context.mesh != None)

class DATA_PT_mesh(DataButtonsPanel):
	__idname__ = "DATA_PT_mesh"
	__label__ = "Mesh"

	def draw(self, context):
		ob = context.object
		mesh = context.mesh
		space = context.space_data
		layout = self.layout

		split = layout.split(percentage=0.65)

		if ob:
			split.template_ID(context, ob, "data")
			split.itemS()
		elif mesh:
			split.template_ID(context, space, "pin_id")
			split.itemS()

		if mesh:
			layout.itemS()

			split = layout.split()
		
			col = split.column()
			col.itemR(mesh, "autosmooth")
			colsub = col.column()
			colsub.active = mesh.autosmooth
			colsub.itemR(mesh, "autosmooth_angle", text="Angle")
			sub = split.column()
			sub.itemR(mesh, "vertex_normal_flip")
			sub.itemR(mesh, "double_sided")
			
			layout.itemR(mesh, "texco_mesh")			
					
bpy.types.register(DATA_PT_mesh)
