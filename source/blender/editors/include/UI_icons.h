/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2009 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file UI_icons.h
 *  \ingroup editorui
 */

/* Note: this is included twice with different #defines for DEF_ICON
   once from UI_resources.h for the internal icon enum and
   once for interface_api.c for the definition of the RNA enum for the icons */

/* ICON_ prefix added */
DEF_ICON(NONE)
DEF_ICON(QUESTION)
DEF_ICON(ERROR)
DEF_ICON(CANCEL)
DEF_ICON(TRIA_RIGHT)
DEF_ICON(TRIA_DOWN)
DEF_ICON(TRIA_LEFT)
DEF_ICON(TRIA_UP)
DEF_ICON(ARROW_LEFTRIGHT)
DEF_ICON(PLUS)
DEF_ICON(DISCLOSURE_TRI_DOWN)
DEF_ICON(DISCLOSURE_TRI_RIGHT)
DEF_ICON(RADIOBUT_OFF)
DEF_ICON(RADIOBUT_ON)
DEF_ICON(MENU_PANEL)
DEF_ICON(BLENDER)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK003)
#endif
DEF_ICON(DOT)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK004)
#endif
DEF_ICON(X)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK005)
#endif
DEF_ICON(GO_LEFT)
DEF_ICON(PLUG)
DEF_ICON(UI)
DEF_ICON(NODE)
DEF_ICON(NODE_SEL)
	
	/* ui */
DEF_ICON(FULLSCREEN)
DEF_ICON(SPLITSCREEN)
DEF_ICON(RIGHTARROW_THIN)
DEF_ICON(BORDERMOVE)
DEF_ICON(VIEWZOOM)
DEF_ICON(ZOOMIN)
DEF_ICON(ZOOMOUT)
DEF_ICON(PANEL_CLOSE)
DEF_ICON(COPY_ID) //ICON_BLANK009
DEF_ICON(EYEDROPPER)
DEF_ICON(LINK_AREA) //ICON_BLANK010
DEF_ICON(AUTO)
DEF_ICON(CHECKBOX_DEHLT)
DEF_ICON(CHECKBOX_HLT)
DEF_ICON(UNLOCKED)
DEF_ICON(LOCKED)
DEF_ICON(UNPINNED)
DEF_ICON(PINNED)
DEF_ICON(SCREEN_BACK)
DEF_ICON(RIGHTARROW)
DEF_ICON(DOWNARROW_HLT)
DEF_ICON(DOTSUP)
DEF_ICON(DOTSDOWN)
DEF_ICON(LINK)
DEF_ICON(INLINK)
DEF_ICON(PLUGIN)
	
	/* various ui */	
DEF_ICON(HELP)
DEF_ICON(GHOST_ENABLED)
DEF_ICON(COLOR)
DEF_ICON(LINKED)
DEF_ICON(UNLINKED)
DEF_ICON(HAND)
DEF_ICON(ZOOM_ALL)
DEF_ICON(ZOOM_SELECTED)
DEF_ICON(ZOOM_PREVIOUS)
DEF_ICON(ZOOM_IN)
DEF_ICON(ZOOM_OUT)
DEF_ICON(RENDER_REGION)
DEF_ICON(BORDER_RECT)
DEF_ICON(BORDER_LASSO)
DEF_ICON(FREEZE)
DEF_ICON(STYLUS_PRESSURE)
DEF_ICON(GHOST_DISABLED)
DEF_ICON(NEW)
DEF_ICON(FILE_TICK)
DEF_ICON(QUIT)
DEF_ICON(URL)
DEF_ICON(RECOVER_LAST)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK038)
#endif
DEF_ICON(FULLSCREEN_ENTER)
DEF_ICON(FULLSCREEN_EXIT)
DEF_ICON(BLANK1)	// Not actually blank - this is used all over the place
	
	/* BUTTONS */
DEF_ICON(LAMP)
DEF_ICON(MATERIAL)
DEF_ICON(TEXTURE)
DEF_ICON(ANIM)
DEF_ICON(WORLD)
DEF_ICON(SCENE)
DEF_ICON(EDIT)
DEF_ICON(GAME)
DEF_ICON(RADIO)
DEF_ICON(SCRIPT)
DEF_ICON(PARTICLES)
DEF_ICON(PHYSICS)
DEF_ICON(SPEAKER)
DEF_ICON(TEXTURE_SHADED) //ICON_BLANK041
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK042)
	DEF_ICON(BLANK043)
	DEF_ICON(BLANK044)
	DEF_ICON(BLANK045)
	DEF_ICON(BLANK046)
	DEF_ICON(BLANK047)
	DEF_ICON(BLANK048)
	DEF_ICON(BLANK049)
	DEF_ICON(BLANK050)
	DEF_ICON(BLANK051)
	DEF_ICON(BLANK052)
	DEF_ICON(BLANK052b)
#endif
	/* EDITORS */
DEF_ICON(VIEW3D)
DEF_ICON(IPO)
DEF_ICON(OOPS)
DEF_ICON(BUTS)
DEF_ICON(FILESEL)
DEF_ICON(IMAGE_COL)
DEF_ICON(INFO)
DEF_ICON(SEQUENCE)
DEF_ICON(TEXT)
DEF_ICON(IMASEL)
DEF_ICON(SOUND)
DEF_ICON(ACTION)
DEF_ICON(NLA)
DEF_ICON(SCRIPTWIN)
DEF_ICON(TIME)
DEF_ICON(NODETREE)
DEF_ICON(LOGIC)
DEF_ICON(CONSOLE)
DEF_ICON(PREFERENCES)
DEF_ICON(CLIP)
DEF_ICON(ASSET_MANAGER)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK057)
	DEF_ICON(BLANK058)
	DEF_ICON(BLANK059)
	DEF_ICON(BLANK060)
	DEF_ICON(BLANK061)
#endif

	/* MODES */
DEF_ICON(OBJECT_DATAMODE)	// XXX fix this up
DEF_ICON(EDITMODE_HLT)
DEF_ICON(FACESEL_HLT)
DEF_ICON(VPAINT_HLT)
DEF_ICON(TPAINT_HLT)
DEF_ICON(WPAINT_HLT)
DEF_ICON(SCULPTMODE_HLT)
DEF_ICON(POSE_HLT)
DEF_ICON(PARTICLEMODE)
DEF_ICON(LIGHTPAINT)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK063)
	DEF_ICON(BLANK064)
	DEF_ICON(BLANK065)
	DEF_ICON(BLANK066)
	DEF_ICON(BLANK067)
	DEF_ICON(BLANK068)
	DEF_ICON(BLANK069)
	DEF_ICON(BLANK070)
	DEF_ICON(BLANK071)
	DEF_ICON(BLANK072)
	DEF_ICON(BLANK073)
	DEF_ICON(BLANK074)
	DEF_ICON(BLANK075)
	DEF_ICON(BLANK076)
	DEF_ICON(BLANK077)
	DEF_ICON(BLANK077b)
#endif
	
	/* DATA */
DEF_ICON(SCENE_DATA)
DEF_ICON(RENDERLAYERS)
DEF_ICON(WORLD_DATA)
DEF_ICON(OBJECT_DATA)
DEF_ICON(MESH_DATA)
DEF_ICON(CURVE_DATA)
DEF_ICON(META_DATA)
DEF_ICON(LATTICE_DATA)
DEF_ICON(LAMP_DATA)
DEF_ICON(MATERIAL_DATA)
DEF_ICON(TEXTURE_DATA)
DEF_ICON(ANIM_DATA)
DEF_ICON(CAMERA_DATA)
DEF_ICON(PARTICLE_DATA)
DEF_ICON(LIBRARY_DATA_DIRECT)
DEF_ICON(GROUP)
DEF_ICON(ARMATURE_DATA)
DEF_ICON(POSE_DATA)
DEF_ICON(BONE_DATA)
DEF_ICON(CONSTRAINT)
DEF_ICON(SHAPEKEY_DATA)
DEF_ICON(CONSTRAINT_BONE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK079)
#endif
DEF_ICON(PACKAGE)
DEF_ICON(UGLYPACKAGE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK079b)
#endif

	/* DATA */
DEF_ICON(BRUSH_DATA)
DEF_ICON(IMAGE_DATA)
DEF_ICON(FILE)
DEF_ICON(FCURVE)
DEF_ICON(FONT_DATA)
DEF_ICON(RENDER_RESULT)
DEF_ICON(SURFACE_DATA)
DEF_ICON(EMPTY_DATA)
DEF_ICON(SETTINGS)
DEF_ICON(RENDER_ANIMATION)
DEF_ICON(RENDER_STILL)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK080F)
#endif
DEF_ICON(BOIDS)
DEF_ICON(STRANDS)
DEF_ICON(LIBRARY_DATA_INDIRECT)
DEF_ICON(GREASEPENCIL)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK083)
	DEF_ICON(BLANK084)
#endif
DEF_ICON(GROUP_BONE)
DEF_ICON(GROUP_VERTEX)
DEF_ICON(GROUP_VCOL)
DEF_ICON(GROUP_UVS)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK089)
	DEF_ICON(BLANK090)
#endif
DEF_ICON(RNA)
DEF_ICON(RNA_ADD)

	/* available */
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK092)
	DEF_ICON(BLANK093)
	DEF_ICON(BLANK094)
	DEF_ICON(BLANK095)
	DEF_ICON(BLANK096)
	DEF_ICON(BLANK097)
	DEF_ICON(BLANK098)
	DEF_ICON(BLANK099)
	DEF_ICON(BLANK100)
	DEF_ICON(BLANK101)
	DEF_ICON(BLANK102)
	DEF_ICON(BLANK103)
	DEF_ICON(BLANK104)
	DEF_ICON(BLANK105)
	DEF_ICON(BLANK106)
	DEF_ICON(BLANK107)
	DEF_ICON(BLANK108)
	DEF_ICON(BLANK109)
	DEF_ICON(BLANK110)
	DEF_ICON(BLANK111)
	DEF_ICON(BLANK112)
	DEF_ICON(BLANK113)
	DEF_ICON(BLANK114)
	DEF_ICON(BLANK115)
	DEF_ICON(BLANK116)
	DEF_ICON(BLANK116b)
#endif
	
	/* OUTLINER */
DEF_ICON(OUTLINER_OB_EMPTY)
DEF_ICON(OUTLINER_OB_MESH)
DEF_ICON(OUTLINER_OB_CURVE)
DEF_ICON(OUTLINER_OB_LATTICE)
DEF_ICON(OUTLINER_OB_META)
DEF_ICON(OUTLINER_OB_LAMP)
DEF_ICON(OUTLINER_OB_CAMERA)
DEF_ICON(OUTLINER_OB_ARMATURE)
DEF_ICON(OUTLINER_OB_FONT)
DEF_ICON(OUTLINER_OB_SURFACE)
DEF_ICON(OUTLINER_OB_SPEAKER)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK120)
	DEF_ICON(BLANK121)
	DEF_ICON(BLANK122)
	DEF_ICON(BLANK123)
	DEF_ICON(BLANK124)
	DEF_ICON(BLANK125)
	DEF_ICON(BLANK126)
	DEF_ICON(BLANK127)
#endif
DEF_ICON(RESTRICT_VIEW_OFF)
DEF_ICON(RESTRICT_VIEW_ON)
DEF_ICON(RESTRICT_SELECT_OFF)
DEF_ICON(RESTRICT_SELECT_ON)
DEF_ICON(RESTRICT_RENDER_OFF)
DEF_ICON(RESTRICT_RENDER_ON)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK127b)
#endif

	/* OUTLINER */
DEF_ICON(OUTLINER_DATA_EMPTY)
DEF_ICON(OUTLINER_DATA_MESH)
DEF_ICON(OUTLINER_DATA_CURVE)
DEF_ICON(OUTLINER_DATA_LATTICE)
DEF_ICON(OUTLINER_DATA_META)
DEF_ICON(OUTLINER_DATA_LAMP)
DEF_ICON(OUTLINER_DATA_CAMERA)
DEF_ICON(OUTLINER_DATA_ARMATURE)
DEF_ICON(OUTLINER_DATA_FONT)
DEF_ICON(OUTLINER_DATA_SURFACE)
DEF_ICON(OUTLINER_DATA_SPEAKER)
DEF_ICON(OUTLINER_DATA_POSE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK130)
	DEF_ICON(BLANK131)
	DEF_ICON(BLANK132)
	DEF_ICON(BLANK133)
	DEF_ICON(BLANK134)
	DEF_ICON(BLANK135)
	DEF_ICON(BLANK136)
	DEF_ICON(BLANK137)
	DEF_ICON(BLANK138)
	DEF_ICON(BLANK139)
	DEF_ICON(BLANK140)
	DEF_ICON(BLANK141)
	DEF_ICON(BLANK142)
	DEF_ICON(BLANK142b)
#endif
	
	/* PRIMITIVES */
DEF_ICON(MESH_PLANE)
DEF_ICON(MESH_CUBE)
DEF_ICON(MESH_CIRCLE)
DEF_ICON(MESH_UVSPHERE)
DEF_ICON(MESH_ICOSPHERE)
DEF_ICON(MESH_GRID)
DEF_ICON(MESH_MONKEY)
DEF_ICON(MESH_CYLINDER)
DEF_ICON(MESH_TORUS)
DEF_ICON(MESH_CONE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK610)
	DEF_ICON(BLANK611)
#endif
DEF_ICON(LAMP_POINT)
DEF_ICON(LAMP_SUN)
DEF_ICON(LAMP_SPOT)
DEF_ICON(LAMP_HEMI)
DEF_ICON(LAMP_AREA)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK617)
	DEF_ICON(BLANK618)
#endif
DEF_ICON(META_EMPTY)
DEF_ICON(META_PLANE)
DEF_ICON(META_CUBE)
DEF_ICON(META_BALL)
DEF_ICON(META_ELLIPSOID)
DEF_ICON(META_CAPSULE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK625)
#endif
	
	/* PRIMITIVES */
DEF_ICON(SURFACE_NCURVE)
DEF_ICON(SURFACE_NCIRCLE)
DEF_ICON(SURFACE_NSURFACE)
DEF_ICON(SURFACE_NCYLINDER)
DEF_ICON(SURFACE_NSPHERE)
DEF_ICON(SURFACE_NTORUS)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK636)
	DEF_ICON(BLANK637)
	DEF_ICON(BLANK638)
#endif
DEF_ICON(CURVE_BEZCURVE)
DEF_ICON(CURVE_BEZCIRCLE)
DEF_ICON(CURVE_NCURVE)
DEF_ICON(CURVE_NCIRCLE)
DEF_ICON(CURVE_PATH)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK644)
	DEF_ICON(BLANK645)
	DEF_ICON(BLANK646)
	DEF_ICON(BLANK647)
	DEF_ICON(BLANK648)
	DEF_ICON(BLANK649)
	DEF_ICON(BLANK650)
	DEF_ICON(BLANK651)
	DEF_ICON(BLANK652)
	DEF_ICON(BLANK653)
	DEF_ICON(BLANK654)
	DEF_ICON(BLANK655)
#endif
	
	/* EMPTY */
DEF_ICON(FORCE_FORCE)
DEF_ICON(FORCE_WIND)
DEF_ICON(FORCE_VORTEX)
DEF_ICON(FORCE_MAGNETIC)
DEF_ICON(FORCE_HARMONIC)
DEF_ICON(FORCE_CHARGE)
DEF_ICON(FORCE_LENNARDJONES)
DEF_ICON(FORCE_TEXTURE)
DEF_ICON(FORCE_CURVE)
DEF_ICON(FORCE_BOID)
DEF_ICON(FORCE_TURBULENCE)
DEF_ICON(FORCE_DRAG)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK672)
	DEF_ICON(BLANK673)
	DEF_ICON(BLANK674)
	DEF_ICON(BLANK675)
	DEF_ICON(BLANK676)
	DEF_ICON(BLANK677)
	DEF_ICON(BLANK678)
	DEF_ICON(BLANK679)
	DEF_ICON(BLANK680)
	DEF_ICON(BLANK681)
	DEF_ICON(BLANK682)
	DEF_ICON(BLANK683)
	DEF_ICON(BLANK684)
	DEF_ICON(BLANK685)

	/* EMPTY */
	DEF_ICON(BLANK690)
	DEF_ICON(BLANK691)
	DEF_ICON(BLANK692)
	DEF_ICON(BLANK693)
	DEF_ICON(BLANK694)
	DEF_ICON(BLANK695)
	DEF_ICON(BLANK696)
	DEF_ICON(BLANK697)
	DEF_ICON(BLANK698)
	DEF_ICON(BLANK699)
	DEF_ICON(BLANK700)
	DEF_ICON(BLANK701)
	DEF_ICON(BLANK702)
	DEF_ICON(BLANK703)
	DEF_ICON(BLANK704)
	DEF_ICON(BLANK705)
	DEF_ICON(BLANK706)
	DEF_ICON(BLANK707)
	DEF_ICON(BLANK708)
	DEF_ICON(BLANK709)
	DEF_ICON(BLANK710)
	DEF_ICON(BLANK711)
	DEF_ICON(BLANK712)
	DEF_ICON(BLANK713)
	DEF_ICON(BLANK714)
	DEF_ICON(BLANK715)
	
	/* EMPTY */
	DEF_ICON(BLANK720)
	DEF_ICON(BLANK721)
	DEF_ICON(BLANK722)
	DEF_ICON(BLANK733)
	DEF_ICON(BLANK734)
	DEF_ICON(BLANK735)
	DEF_ICON(BLANK736)
	DEF_ICON(BLANK737)
	DEF_ICON(BLANK738)
	DEF_ICON(BLANK739)
	DEF_ICON(BLANK740)
	DEF_ICON(BLANK741)
	DEF_ICON(BLANK742)
	DEF_ICON(BLANK743)
	DEF_ICON(BLANK744)
	DEF_ICON(BLANK745)
	DEF_ICON(BLANK746)
	DEF_ICON(BLANK747)
	DEF_ICON(BLANK748)
	DEF_ICON(BLANK749)
	DEF_ICON(BLANK750)
	DEF_ICON(BLANK751)
	DEF_ICON(BLANK752)
	DEF_ICON(BLANK753)
	DEF_ICON(BLANK754)
	DEF_ICON(BLANK755)

	/* EMPTY */
	DEF_ICON(BLANK760)
	DEF_ICON(BLANK761)
	DEF_ICON(BLANK762)
	DEF_ICON(BLANK763)
	DEF_ICON(BLANK764)
	DEF_ICON(BLANK765)
	DEF_ICON(BLANK766)
	DEF_ICON(BLANK767)
	DEF_ICON(BLANK768)
	DEF_ICON(BLANK769)
	DEF_ICON(BLANK770)
	DEF_ICON(BLANK771)
	DEF_ICON(BLANK772)
	DEF_ICON(BLANK773)
	DEF_ICON(BLANK774)
	DEF_ICON(BLANK775)
	DEF_ICON(BLANK776)
	DEF_ICON(BLANK777)
	DEF_ICON(BLANK778)
	DEF_ICON(BLANK779)
	DEF_ICON(BLANK780)
	DEF_ICON(BLANK781)
	DEF_ICON(BLANK782)
	DEF_ICON(BLANK783)
	DEF_ICON(BLANK784)
	DEF_ICON(BLANK785)
#endif

	/* MODIFIERS */
DEF_ICON(MODIFIER)
DEF_ICON(MOD_WAVE)
DEF_ICON(MOD_BUILD)
DEF_ICON(MOD_DECIM)
DEF_ICON(MOD_MIRROR)
DEF_ICON(MOD_SOFT)
DEF_ICON(MOD_SUBSURF)
DEF_ICON(HOOK)
DEF_ICON(MOD_PHYSICS)
DEF_ICON(MOD_PARTICLES)
DEF_ICON(MOD_BOOLEAN)
DEF_ICON(MOD_EDGESPLIT)
DEF_ICON(MOD_ARRAY)
DEF_ICON(MOD_UVPROJECT)
DEF_ICON(MOD_DISPLACE)
DEF_ICON(MOD_CURVE)
DEF_ICON(MOD_LATTICE)
DEF_ICON(CONSTRAINT_DATA)
DEF_ICON(MOD_ARMATURE)
DEF_ICON(MOD_SHRINKWRAP)
DEF_ICON(MOD_CAST)
DEF_ICON(MOD_MESHDEFORM)
DEF_ICON(MOD_BEVEL)
DEF_ICON(MOD_SMOOTH)
DEF_ICON(MOD_SIMPLEDEFORM)
DEF_ICON(MOD_MASK)

	/* MODIFIERS */
DEF_ICON(MOD_CLOTH)
DEF_ICON(MOD_EXPLODE)
DEF_ICON(MOD_FLUIDSIM)
DEF_ICON(MOD_MULTIRES)
DEF_ICON(MOD_SMOKE)
DEF_ICON(MOD_SOLIDIFY)
DEF_ICON(MOD_SCREW)
DEF_ICON(MOD_VERTEX_WEIGHT)
DEF_ICON(MOD_DYNAMICPAINT)
DEF_ICON(MOD_REMESH)
DEF_ICON(MOD_OCEAN)
DEF_ICON(MOD_WARP)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK165)
	DEF_ICON(BLANK166)
	DEF_ICON(BLANK167)
	DEF_ICON(BLANK168)
	DEF_ICON(BLANK169)
	DEF_ICON(BLANK170)
	DEF_ICON(BLANK171)
	DEF_ICON(BLANK172)
	DEF_ICON(BLANK173)
	DEF_ICON(BLANK174)
	DEF_ICON(BLANK175)
	DEF_ICON(BLANK176)
	DEF_ICON(BLANK177)
	DEF_ICON(BLANK177b)
#endif
	
	/* ANIMATION */
DEF_ICON(REC)
DEF_ICON(PLAY)
DEF_ICON(FF)
DEF_ICON(REW)
DEF_ICON(PAUSE)
DEF_ICON(PREV_KEYFRAME)
DEF_ICON(NEXT_KEYFRAME)
DEF_ICON(PLAY_AUDIO)
DEF_ICON(PLAY_REVERSE)
DEF_ICON(PREVIEW_RANGE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK180)
#endif
DEF_ICON(PMARKER_ACT)
DEF_ICON(PMARKER_SEL)
DEF_ICON(PMARKER)
DEF_ICON(MARKER_HLT)
DEF_ICON(MARKER)
DEF_ICON(SPACE2)	// XXX
DEF_ICON(SPACE3)	// XXX
DEF_ICON(KEYINGSET)
DEF_ICON(KEY_DEHLT)
DEF_ICON(KEY_HLT)
DEF_ICON(MUTE_IPO_OFF)
DEF_ICON(MUTE_IPO_ON)
DEF_ICON(VISIBLE_IPO_OFF)
DEF_ICON(VISIBLE_IPO_ON)
DEF_ICON(DRIVER)

	/* ANIMATION */
DEF_ICON(SOLO_OFF)
DEF_ICON(SOLO_ON)
DEF_ICON(FRAME_PREV)
DEF_ICON(FRAME_NEXT)
#ifndef DEF_ICON_BLANK_SKIP
	/* available */
	DEF_ICON(BLANK186)
	DEF_ICON(BLANK187)
	DEF_ICON(BLANK188)
	DEF_ICON(BLANK189)
	DEF_ICON(BLANK190)
	DEF_ICON(BLANK191)
	DEF_ICON(BLANK192)
	DEF_ICON(BLANK193)
	DEF_ICON(BLANK194)
	DEF_ICON(BLANK195)
	DEF_ICON(BLANK196)
	DEF_ICON(BLANK197)
	DEF_ICON(BLANK198)
	DEF_ICON(BLANK199)
	DEF_ICON(BLANK200)
	DEF_ICON(BLANK201)
	DEF_ICON(BLANK202)
	DEF_ICON(BLANK203)
	DEF_ICON(BLANK204)
	DEF_ICON(BLANK205)
	DEF_ICON(BLANK206)
	DEF_ICON(BLANK207)
#endif

	/* EDITING */
DEF_ICON(VERTEXSEL)
DEF_ICON(EDGESEL)
DEF_ICON(FACESEL)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK209)
	DEF_ICON(BLANK210)
#endif
DEF_ICON(ROTATE)
DEF_ICON(CURSOR)
DEF_ICON(ROTATECOLLECTION)
DEF_ICON(ROTATECENTER)
DEF_ICON(ROTACTIVE)
DEF_ICON(ALIGN)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK211)
#endif
DEF_ICON(SMOOTHCURVE)
DEF_ICON(SPHERECURVE)
DEF_ICON(ROOTCURVE)
DEF_ICON(SHARPCURVE)
DEF_ICON(LINCURVE)
DEF_ICON(NOCURVE)
DEF_ICON(RNDCURVE)
DEF_ICON(PROP_OFF)
DEF_ICON(PROP_ON)
DEF_ICON(PROP_CON)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK212)
#endif
DEF_ICON(PARTICLE_POINT)
DEF_ICON(PARTICLE_TIP)
DEF_ICON(PARTICLE_PATH)
	
	/* EDITING */
DEF_ICON(MAN_TRANS)
DEF_ICON(MAN_ROT)
DEF_ICON(MAN_SCALE)
DEF_ICON(MANIPUL)
DEF_ICON(SNAP_OFF)
DEF_ICON(SNAP_ON)
DEF_ICON(SNAP_NORMAL)
DEF_ICON(SNAP_INCREMENT)
DEF_ICON(SNAP_VERTEX)
DEF_ICON(SNAP_EDGE)
DEF_ICON(SNAP_FACE)
DEF_ICON(SNAP_VOLUME)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK220)
#endif
DEF_ICON(STICKY_UVS_LOC)
DEF_ICON(STICKY_UVS_DISABLE)
DEF_ICON(STICKY_UVS_VERT)
DEF_ICON(CLIPUV_DEHLT)
DEF_ICON(CLIPUV_HLT)
DEF_ICON(SNAP_PEEL_OBJECT)
DEF_ICON(GRID)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK221)
	DEF_ICON(BLANK222)
	DEF_ICON(BLANK224)
	DEF_ICON(BLANK225)
	DEF_ICON(BLANK226)
	DEF_ICON(BLANK226b)
#endif

	/* EDITING */
DEF_ICON(PASTEDOWN)
DEF_ICON(COPYDOWN)
DEF_ICON(PASTEFLIPUP)
DEF_ICON(PASTEFLIPDOWN)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK227)
	DEF_ICON(BLANK228)
	DEF_ICON(BLANK229)
	DEF_ICON(BLANK230)
#endif
DEF_ICON(SNAP_SURFACE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK232)
	DEF_ICON(BLANK233)
#endif
DEF_ICON(RETOPO)
DEF_ICON(UV_VERTEXSEL)
DEF_ICON(UV_EDGESEL)
DEF_ICON(UV_FACESEL)
DEF_ICON(UV_ISLANDSEL)
DEF_ICON(UV_SYNC_SELECT)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK240)
	DEF_ICON(BLANK241)
	DEF_ICON(BLANK242)
	DEF_ICON(BLANK243)
	DEF_ICON(BLANK244)
	DEF_ICON(BLANK245)
	DEF_ICON(BLANK246)
	DEF_ICON(BLANK247)
	DEF_ICON(BLANK247b)
#endif
	
	/* 3D VIEW */
DEF_ICON(BBOX)
DEF_ICON(WIRE)
DEF_ICON(SOLID)
DEF_ICON(SMOOTH)
DEF_ICON(POTATO)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK248)
#endif
DEF_ICON(ORTHO)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK249)
	DEF_ICON(BLANK250)
#endif
DEF_ICON(LOCKVIEW_OFF)
DEF_ICON(LOCKVIEW_ON)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK251)
#endif
DEF_ICON(AXIS_SIDE)
DEF_ICON(AXIS_FRONT)
DEF_ICON(AXIS_TOP)
DEF_ICON(NDOF_DOM)
DEF_ICON(NDOF_TURN)
DEF_ICON(NDOF_FLY)
DEF_ICON(NDOF_TRANS)
DEF_ICON(LAYER_USED)
DEF_ICON(LAYER_ACTIVE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK254)
	DEF_ICON(BLANK255)
	DEF_ICON(BLANK256)
	DEF_ICON(BLANK257)
	DEF_ICON(BLANK257b)

	/* available */
	DEF_ICON(BLANK258)
	DEF_ICON(BLANK259)
	DEF_ICON(BLANK260)
	DEF_ICON(BLANK261)
	DEF_ICON(BLANK262)
	DEF_ICON(BLANK263)
	DEF_ICON(BLANK264)
	DEF_ICON(BLANK265)
	DEF_ICON(BLANK266)
	DEF_ICON(BLANK267)
	DEF_ICON(BLANK268)
	DEF_ICON(BLANK269)
	DEF_ICON(BLANK270)
	DEF_ICON(BLANK271)
	DEF_ICON(BLANK272)
	DEF_ICON(BLANK273)
	DEF_ICON(BLANK274)
	DEF_ICON(BLANK275)
	DEF_ICON(BLANK276)
	DEF_ICON(BLANK277)
	DEF_ICON(BLANK278)
	DEF_ICON(BLANK279)
	DEF_ICON(BLANK280)
	DEF_ICON(BLANK281)
	DEF_ICON(BLANK282)
	DEF_ICON(BLANK282b)
#endif

	/* FILE SELECT */
DEF_ICON(SORTALPHA)
DEF_ICON(SORTBYEXT)
DEF_ICON(SORTTIME)
DEF_ICON(SORTSIZE)
DEF_ICON(LONGDISPLAY)
DEF_ICON(SHORTDISPLAY)
DEF_ICON(GHOST)
DEF_ICON(IMGDISPLAY)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK284)
	DEF_ICON(BLANK285)
#endif
DEF_ICON(BOOKMARKS)
DEF_ICON(FONTPREVIEW)
DEF_ICON(FILTER)
DEF_ICON(NEWFOLDER)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK285F)
#endif
DEF_ICON(FILE_PARENT)
DEF_ICON(FILE_REFRESH)
DEF_ICON(FILE_FOLDER)
DEF_ICON(FILE_BLANK)
DEF_ICON(FILE_BLEND)
DEF_ICON(FILE_IMAGE)
DEF_ICON(FILE_MOVIE)
DEF_ICON(FILE_SCRIPT)
DEF_ICON(FILE_SOUND)
DEF_ICON(FILE_FONT)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK291b)

	/* available */
	DEF_ICON(BLANK292)
	DEF_ICON(BLANK293)
	DEF_ICON(BLANK294)
	DEF_ICON(BLANK295)
	DEF_ICON(BLANK296)
	DEF_ICON(BLANK297)
	DEF_ICON(BLANK298)
	DEF_ICON(BLANK299)
	DEF_ICON(BLANK300)
	DEF_ICON(BLANK301)
	DEF_ICON(BLANK302)
	DEF_ICON(BLANK303)
	DEF_ICON(BLANK304)
	DEF_ICON(BLANK305)
	DEF_ICON(BLANK306)
#endif
DEF_ICON(BACK)
DEF_ICON(FORWARD)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK309)
	DEF_ICON(BLANK310)
	DEF_ICON(BLANK311)
	DEF_ICON(BLANK312)
	DEF_ICON(BLANK313)
	DEF_ICON(BLANK314)
	DEF_ICON(BLANK315)
	DEF_ICON(BLANK316)
#endif
DEF_ICON(DISK_DRIVE)
	
	/* SHADING / TEXT */
DEF_ICON(MATPLANE)
DEF_ICON(MATSPHERE)
DEF_ICON(MATCUBE)
DEF_ICON(MONKEY)
DEF_ICON(HAIR)
DEF_ICON(ALIASED)
DEF_ICON(ANTIALIASED)
DEF_ICON(MAT_SPHERE_SKY)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK319)
	DEF_ICON(BLANK320)
	DEF_ICON(BLANK321)
	DEF_ICON(BLANK322)
#endif
DEF_ICON(WORDWRAP_OFF)
DEF_ICON(WORDWRAP_ON)
DEF_ICON(SYNTAX_OFF)
DEF_ICON(SYNTAX_ON)
DEF_ICON(LINENUMBERS_OFF)
DEF_ICON(LINENUMBERS_ON)
DEF_ICON(SCRIPTPLUGINS)		// XXX CREATE NEW
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK323)
	DEF_ICON(BLANK324)
	DEF_ICON(BLANK325)
	DEF_ICON(BLANK326)
	DEF_ICON(BLANK327)
	DEF_ICON(BLANK328)
	DEF_ICON(BLANK328b)
#endif
	
	/* SEQUENCE / IMAGE EDITOR */
DEF_ICON(SEQ_SEQUENCER)
DEF_ICON(SEQ_PREVIEW)
DEF_ICON(SEQ_LUMA_WAVEFORM)
DEF_ICON(SEQ_CHROMA_SCOPE)
DEF_ICON(SEQ_HISTOGRAM)
DEF_ICON(SEQ_SPLITVIEW)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK331)
	DEF_ICON(BLANK332)
	DEF_ICON(BLANK333)
#endif
DEF_ICON(IMAGE_RGB)	// XXX CHANGE TO STRAIGHT ALPHA, Z ETC
DEF_ICON(IMAGE_RGB_ALPHA)
DEF_ICON(IMAGE_ALPHA)
DEF_ICON(IMAGE_ZDEPTH)
DEF_ICON(IMAGEFILE)
#ifndef DEF_ICON_BLANK_SKIP
	DEF_ICON(BLANK336)
	DEF_ICON(BLANK337)
	DEF_ICON(BLANK338)
	DEF_ICON(BLANK339)
	DEF_ICON(BLANK340)
	DEF_ICON(BLANK341)
	DEF_ICON(BLANK342)
	DEF_ICON(BLANK343)
	DEF_ICON(BLANK344)
	DEF_ICON(BLANK345)
	DEF_ICON(BLANK346)
	DEF_ICON(BLANK346b)
#endif

	/* brush icons */

DEF_ICON(BRUSH_ADD)
DEF_ICON(BRUSH_BLOB)
DEF_ICON(BRUSH_BLUR)
DEF_ICON(BRUSH_CLAY)
DEF_ICON(BRUSH_CLONE)
DEF_ICON(BRUSH_CREASE)
DEF_ICON(BRUSH_DARKEN)
DEF_ICON(BRUSH_FILL)
DEF_ICON(BRUSH_FLATTEN)
DEF_ICON(BRUSH_GRAB)
DEF_ICON(BRUSH_INFLATE)
DEF_ICON(BRUSH_LAYER)
DEF_ICON(BRUSH_LIGHTEN)
DEF_ICON(BRUSH_MIX)
DEF_ICON(BRUSH_MULTIPLY)
DEF_ICON(BRUSH_NUDGE)
DEF_ICON(BRUSH_PINCH)
DEF_ICON(BRUSH_SCRAPE)
DEF_ICON(BRUSH_SCULPT_DRAW)
DEF_ICON(BRUSH_SMEAR)
DEF_ICON(BRUSH_SMOOTH)
DEF_ICON(BRUSH_SNAKE_HOOK)
DEF_ICON(BRUSH_SOFTEN)
DEF_ICON(BRUSH_SUBTRACT)
DEF_ICON(BRUSH_TEXDRAW)
DEF_ICON(BRUSH_THUMB)
DEF_ICON(BRUSH_ROTATE)
DEF_ICON(BRUSH_VERTEXDRAW)

/* vector icons, VICO_ prefix added */	
DEF_VICO(VIEW3D_VEC)
DEF_VICO(EDIT_VEC)
DEF_VICO(EDITMODE_DEHLT)
DEF_VICO(EDITMODE_HLT)
DEF_VICO(DISCLOSURE_TRI_RIGHT_VEC)
DEF_VICO(DISCLOSURE_TRI_DOWN_VEC)
DEF_VICO(MOVE_UP_VEC)
DEF_VICO(MOVE_DOWN_VEC)
DEF_VICO(X_VEC)
DEF_VICO(SMALL_TRI_RIGHT_VEC)
