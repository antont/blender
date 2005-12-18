#!BPY

"""
Name: 'COLLADA (.dae) ...'
Blender: 237
Group: 'Export'
Tooltip: 'Export scene to COLLADA format (.dae)'
"""

__author__ = "Mikael Lagre"
__url__ = ("blender", "elysiun", "Project homepage, http://colladablender.sourceforge.net", "Official Collada site, http://www.collada.org")
__version__ = "0.2"
__bpydoc__ = """
Description: Exports Blender scene to the COLLADA 1.3.1 format.

Usage: Run the script from the menu or inside Blender.  

Notes: Does not export animation.
"""

# --------------------------------------------------------------------------
# Collada exporter version 0.2
# --------------------------------------------------------------------------
# ***** BEGIN GPL LICENSE BLOCK *****
#
# Copyright (C) 2005: Mikael Lagre' contactme@mikaellagre.com
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****
# --------------------------------------------------------------------------





_ERROR = False

try:
    import math
except:
    print "Error! Could not find math module"
    _ERROR = True

try:
    import Blender
    from Blender import *
except:
    print "Error! Could not find Blender modules!"
    _ERROR = True
try:
    from xml.dom.minidom import Document, Element, Childless, Text, _write_data
except:
    print "Error! Could not find XML modules!"
    _ERROR = True


# === GLOBAL EXPORT SETTINGS ===
# (features below are coming in v0.04)
# exportSelectedOnly = False
# bakeTransforms = False
# ==============================
# Export options

exportBakedTransform = False
exportSelected = False
exportCameraAsLookAt = False
niceFormat = False

# ----------------------------------------------
# Class structures
# ----------------------------------------------
class COLLADADocument( Document ):
    """ A COLLADA Document ( DAE - Digital Asset Exangche) """
    __file_ref = None    
    
    def openFile( self, file ):
        self.__file_ref = open( file, "w" )
        return
    
    def buildXML( self ):
        if not ( self.__file_ref == None ):
            self.writexml( self.__file_ref, "", "\t", '\n', "utf-8" )
        return

    def cleanUp( self ):
        self.unlink()
        self.__file_ref.close()
        return
    
    # Definition of create[ColladaElementTypes]
    def createCOLLADAElement( self, version=None ):
        e = _COLLADAElement( version )
        e.ownerDocument = self
        return e

    def createSceneElement( self, id=None, name=None ):
        e = _SceneElement( id, name )
        e.ownerDocument = self
        return e
        
    def createNodeElement( self, name=None, id=None, nodeType=None ):
        e = _NodeElement( name, id, nodeType )
        e.ownerDocument = self
        return e

    def createBoundingBoxElement( self ):
        e = _BoundingBoxElement( )
        e.ownerDocument = self
        return e

    def createCameraElement( self, id=None, name=None ):
        e = _CameraElement( id, name )
        e.ownerDocument = self
        return e

    def createOpticsElement( self ):
        e = _OpticsElement( )
        e.ownerDocument = self
        return e

    def createImagerElement( self ):
        e = _ImagerElement( )
        e.ownerDocument = self
        return e

    def createInstanceElement( self, url ):
        e = _InstanceElement( url )
        e.ownerDocument = self
        return e

    def createLibraryElement( self, type ):
        e = _LibraryElement( type )
        e.ownerDocument = self
        return e

    def createChannelElement( self ):
        e = _ChannelElement( )
        e.ownerDocument = self
        return e

    def createSamplerElement( self ):
        e = _SamplerElement( )
        e.ownerDocument = self
        return e

    def createControllerElement( self ):
        e = _ControllerElement( )
        e.ownerDocument = self
        return e

    def createSkinElement( self ):
        e = _SkinElement( )
        e.ownerDocument = self
        return e

    def createCombinerElement( self ):
        e = _CombinerElement( )
        e.ownerDocument = self
        return e

    def createJointsElement( self ):
        e = _JointsElement( )
        e.ownerDocument = self
        return e

    def createGeometryElement( self, id=None, name=None ):
        e = _GeometryElement( id, name )
        e.ownerDocument = self
        return e

    def createMeshElement( self, id=None, name=None  ):
        e = _MeshElement( id, name )
        e.ownerDocument = self
        return e
   
    def createLinesElement( self ):
        e = _LinesElement( )
        e.ownerDocument = self
        return e

    def createLinestripsElement( self ):
        e = _LinestripsElement( )
        e.ownerDocument = self
        return e

    def createPolygonsElement( self, count=0, material=None ):
        e = _PolygonsElement( count, material )
        e.ownerDocument = self
        return e
    
    def createPElement( self ):
        e = _PElement( )
        e.ownerDocument = self
        return e
    
    def createTrianglesElement( self ):
        e = _TrianglesElement( )
        e.ownerDocument = self
        return e

    def createTrifansElement( self ):
        e = _TrifansElement( )
        e.ownerDocument = self
        return e

    def createTristripsElement( self ):
        e = _TristripsElement( )
        e.ownerDocument = self
        return e

    def createVerticesElement( self, id, name, count ):
        e = _VerticesElement( id, name, count )
        e.ownerDocument = self
        return e

    def createSourceElement( self, id=None, name=None ):
        e = _SourceElement( id, name )
        e.ownerDocument = self
        return e

    def createAccessorElement( self, count=0, source="", id=None, offset=None, stride=None ):
        e = _AccessorElement( count, id, offset, source, stride)
        e.ownerDocument = self
        return e

    def createArrayElement( self ):
        e = _ArrayElement( )
        e.ownerDocument = self
        return e

    def createBoolArrayElement( self ):
        e = _BoolArrayElement( )
        e.ownerDocument = self
        return e

    def createFloatArrayElement( self, count, id=None, name=None, digits=None, 
                                    magnitude=None ):
        e = _FloatArrayElement( count, id, name, digits, magnitude)
        e.ownerDocument = self
        return e

    def createIntArrayElement( self ):
        e = _IntArrayElement( )
        e.ownerDocument = self
        return e

    def createNameArrayElement( self ):
        e = _NameArrayElement( )
        e.ownerDocument = self
        return e

    def createInputElement( self, idx=None, semantic=None, source=None ):
        e = _InputElement( idx, semantic, source )
        e.ownerDocument = self
        return e

    def createMaterialElement( self, id=None, name=None ):
        e = _MaterialElement( id, name )
        e.ownerDocument = self
        return e

    def createShaderElement( self, id=None, name=None ):
        e = _ShaderElement( id, name )
        e.ownerDocument = self
        return e

    def createPassElement( self ):
        e = _PassElement( )
        e.ownerDocument = self
        return e

    def createTechniqueElement( self, profile=None ):
        e = _TechniqueElement( profile )
        e.ownerDocument = self
        return e

    def createImageElement( self, id=None, name=None, height=None, width=None, 
                                depth=None, source=None, format=None ):
        e = _ImageElement( id, name, height, width, depth, source, format )
        e.ownerDocument = self
        return e

    def createLightElement( self, id=None, name=None, type=None ):
        e = _LightElement( id, name, type )
        e.ownerDocument = self
        return e

    def createTextureElement( self, id=None, name=None ):
        e = _TextureElement( id, name,  )
        e.ownerDocument = self
        return e

    def createProgramElement( self, id=None, name=None, url=None ):
        e = _ProgramElement( id, name, url )
        e.ownerDocument = self
        return e

    def createCodeElement( self ):
        e = _CodeElement( )
        e.ownerDocument = self
        return e

    def createEntryElement( self ):
        e = _EntryElement( )
        e.ownerDocument = self
        return e

    def createParamElement( self, id=None, name=None, type=None, flow=None, 
                            semantic=None, sid=None ):
        e = _ParamElement( id, name, type, flow, semantic, sid )
        e.ownerDocument = self
        return e

    def createMatrixElement( self, sid=None ):
        e = _MatrixElement( sid )
        e.ownerDocument = self
        return e

    def createLookAtElement( self ):
        e = _LookAtElement( )
        e.ownerDocument = self
        return e

    def createPerspectiveElement( self ):
        e = _PerspectiveElement( )
        e.ownerDocument = self
        return e

    def createRotateElement( self, sid=None ):
        e = _RotateElement( sid )
        e.ownerDocument = self
        return e

    def createScaleElement( self, sid=None ):
        e = _ScaleElement( sid )
        e.ownerDocument = self
        return e

    def createSkewElement( self ):
        e = _SkewElement( )
        e.ownerDocument = self
        return e

    def createTranslateElement( self, sid=None ):
        e = _TranslateElement( sid )
        e.ownerDocument = self
        return e

    def createAssetElement( self ):
        e = _AssetElement( )
        e.ownerDocument = self
        return e

    def createAuthorElement( self, data=None ):
        e = _AuthorElement( data )
        e.ownerDocument = self
        return e
    
    def createAuthoringToolElement( self, data=None ):
        e = _AuthoringToolElement( data )
        e.ownerDocument = self
        return e

    def createCreatedElement( self, data=None ):
        e = _CreatedElement( data )
        e.ownerDocument = self
        return e

    def createModifiedElement( self, data=None ):
        e = _ModifiedElement( data )
        e.ownerDocument = self
        return e

    def createRevisionElement( self, data=None ):
        e = _RevisionElement( data )
        e.ownerDocument = self
        return e

    def createSourceDataElement( self, data=None ):
        e = _SourceDataElement( data )
        e.ownerDocument = self
        return e

    def createElement( self, data=None ):
        e = _( data )
        e.ownerDocument = self
        return e

    def createCopyrightElement( self, data=None ):
        e = _CopyrightElement( data )
        e.ownerDocument = self
        return e

    def createTitleElement( self, data=None ):
        e = _TitleElement( data )
        e.ownerDocument = self
        return e

    def createSubjectElement( self, data=None ):
        e = _SubjectElement( data )
        e.ownerDocument = self
        return e

    def createKeywordsElement( self, data=None ):
        e = _KeywordsElement( data )
        e.ownerDocument = self
        return e

    def createCommentsElement( self, data=None ):
        e = _CommentsElement( data )
        e.ownerDocument = self
        return e

    def createUnitElement( self, name=None, meter=None ):
        e = _UnitElement( data )
        e.ownerDocument = self
        return e

    def createUpAxisElement( self, data=None ):
        e = _UpAxisElement( data )
        e.ownerDocument = self
        return e


    def createExtraElement( self ):
        e = _ExtraElement( )
        e.ownerDocument = self
        return e


# COLLADA Common Profile constants strings
# ( for COLLADA specification 1.3.1 )
# Also in this collada common profile class is library type constants
# flow types and other xs:NMTOKEN constants definied in the schema
class _CommonProfile:
    
    COMMON = 1
    BLENDER = 2
    
    str = [ "", "COMMON", "BLENDER" ]
    
    class _ParamName:
        A = 1
        AMBIENT = 2
        ANGLE = 3
        ATTENUATION = 4
        ATTENUATION_SCALE = 5
        B = 6
        BOTTOM = 7
        COLOR = 8
        DIFFUSE = 9
        EMISSION = 10
        FALLOFF = 11
        FALLOFF_SCALE = 12
        G = 13
        LEFT = 14
        P = 15
        Q = 16
        R = 17
        REFLECTIVE = 18
        REFLECTIVITY = 19
        RIGHT = 20
        S = 21
        SHININESS = 22
        SPECULAR = 23
        T = 24
        TANGENT_X = 25
        TANGENT_Y = 26
        TANGENT_Z = 27
        TIME = 28
        TOP = 29
        TRANSPARENCY = 30
        TRANSPARENT = 31
        U = 32
        V = 33
        W = 34
        X = 35
        XFOV = 36
        Y = 37
        YFOV = 38
        Z = 39
        ZFAR = 40
        ZNEAR = 41
        
        str = [ "", "A", "AMBIENT", "ANGLE", "ATTENUATION", 
                "ATTENUATION_SCALE", "B", "BOTTOM", "COLOR", "DIFFUSE",
                "EMISSION", "FALLOFF", "FALLOFF_SCALE", "G", "LEFT",
                "P", "Q", "R", "REFLECTIVE", "REFLECTIVITY", "RIGHT", "S", 
                "SHININESS", "SPECULAR", "T", "TANGENT.X", "TANGENT.Y", 
                "TANGENT.Z", "TIME", "TOP", "TRANSPARENCY", "TRANSPARENT",
                "U", "V", "W", "X", "XFOV", "Y", "YFOV", "Z", "ZFAR", "ZNEAR" ]
    
    class _ProgramIDAndURL:
        ANGLE_MAP = 1
        BEZIER = 2
        BSPLINE = 3
        CARDINAL = 4
        CONSTANT = 5
        CUBE_MAP = 6
        FISH_EYE = 7
        HERMITE = 8
        LAMBERT = 9
        LINEAR = 10
        ORTHOGRAPHIC = 11
        PANORAMA = 12
        PERSPECTIVE = 13
        PHONG = 14
        REAR_FISH_EYE = 15
        SPHERICAL = 16
        
        str = [ "", "ANGLE_MAP", "BEZIER", "BSPLINE", "CARDINAL", "CONSTANT",
                "CUBE_MAP", "FISH_EYE", "HERMITE", "LAMBERT", "LINEAR",
                "ORTHOGRAPHIC", "PANORAMA", "PERSPECTIVE", "PHONG",
                "REAR_FISH_EYE", "SPHERICAL" ]

    class _CodeAndEntrySemantic:
        FRAGMENT_PROGRAM = 1
        VERTEX_PROGRAM = 2
    
    class _InputSemantic:
        BIND_SHAPE_NORMAL = 1
        BIND_SHAPE_POSITION = 2
        BINORMAL = 3
        COLOR = 4
        IMAGE = 5
        INPUT = 6
        IN_TANGENT = 7
        INTERPOLATION = 8
        INV_BIND_MATRIX = 9
        JOINT = 10
        JOINTS_AND_WEIGHTS = 11
        NORMAL = 12
        OUTPUT = 13
        OUT_TANGENT = 14
        POSITION = 15
        TANGENT = 16
        TEXCOORD = 17
        TEXTURE = 18
        UV = 19
        VERTEX = 20
        WEIGHT = 21
        
        str = [ "", "BIND_SHAPE_NORMAL", "BIND_SHAPE_POSITION", "BINORMAL",
                "COLOR", "IMAGE", "INPUT", "IN_TANGENT", "INETRPOLATION", 
                "INV_BIND_MATRIX", "JOINT", "JOINTS_AND_WEIGHTS", "NORMAL",
                "OUTPUT", "OUT_TANGENT", "POSITION", "TANGENT", "TEXCOORD",
                "TEXTURE", "UV", "VERTEX", "WEIGHT" ]
    
    class _ChannelAndControllerTarget:
        #( # )[( # )]
        A = 1
        ANGLE = 2
        B = 3
        G = 4
        P = 5
        Q = 6
        R = 7
        S = 8
        T = 9
        TIME = 10
        U = 11
        V = 12
        W = 13
        X = 14
        Y = 15
        Z = 16

    class _LibraryType:
        ANIMATION = 1
        CAMERA = 2
        CODE = 3
        CONTROLLER = 4
        GEOMETRY = 5
        IMAGE = 6
        LIGHT = 7
        MATERIAL = 8
        PROGRAM = 9
        TEXTURE = 10
        
        str = [ "", "ANIMATION", "CAMERA", "CODE", "CONTROLLER", "GEOMETRY",
                "IMAGE", "LIGHT", "MATERIAL", "PROGRAM", "TEXTURE" ]
    
    
    class _FlowType:
        IN = 1
        OUT = 2
        INOUT = 3
        
        str = [ "", "IN", "OUT", "INOUT" ]

    class _NodeType:
        NODE = 1
        JOINT = 2
        
        str = [ "", "NODE", "JOINT" ]

    class _LightType:
        AMBIENT = 1
        DIRECTIONAL = 2
        POINT = 3
        SPOT = 4
        
        str = [ "", "AMBIENT", "DIRECTIONAL", "POINT", "SPOT" ]
        
    
    # Definition of modules for easier access in code
    # Note: This is pretty stupid I know ...
    
    class PN( _ParamName ):
        pass
    class PIAU( _ProgramIDAndURL ):
        pass
    class CAES( _CodeAndEntrySemantic ):
        pass
    class IS( _InputSemantic ):
        pass
    class CACT( _ChannelAndControllerTarget ):
        pass
    class LT( _LibraryType ):
        pass
    class FT( _FlowType ):
        pass
    class NT( _NodeType ):
        pass
    class LIGHT( _LightType ):
        pass

# Common Profile
class CP( _CommonProfile ):
    pass


# Element types
# Note that attributes are local here defined as their types (string, float)
# and not Attr object types. Every element
# has defined set and get methods for their types to create a better interface
# for its element type

# NOTE: Do not create instances of these class directly. Use the create...
# functions in COLLADADocument

class _COLLADAElement( Element ):
    
    __version = "1.3.1"
    __xmlns = "http://www.collada.org/2005/COLLADASchema"
    __xmlbase = None
    
    def setVersion( self, version ):
        self.__version = version
        self.setAttribute( "version", self.__version )
        return
    
    def getVersion( self ):
        return self.__version
   
    def __init__( self, version=None ):
        Element.__init__( self, "COLLADA" )
        self.__version = version
        if ( self.__version == None ):
            self.__version = "1.3.1"
        self.setAttribute( "version", self.__version )
        self.setAttribute( "xmlns", self.__xmlns )

class _SceneElement( Element ):
    
    _name = ""
    _id = ""
    
    def __init__( self, id, name ):
        Element.__init__( self, "scene" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )
    
class _NodeElement( Element ):
    
    _name = None
    _id = None
    _nodeType = None
    
    def __init__( self, name, id, nodeType ):
        Element.__init__( self, "node" )
        self._id = id
        self._name = name
        self._nodeType = nodeType
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )
        if not ( self._nodeType == None ):
            self.setAttribute( "type", CP.NT.str[ self._nodeType ] )

class _BoundingBoxElement( Element ):
    pass

class _CameraElement( Element ):
    
    _name = ""
    _id = ""
    
    def __init__( self, id, name ):
        Element.__init__( self, "camera" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )

class _OpticsElement( Element) :
    
    def __init__( self ):
        Element.__init__( self, "optics" )


class _ImagerElement( Element ):
    pass

class _InstanceElement( Element, Childless ):
    
    _url = None
    
    def __init__( self, url ):
        Element.__init__( self, "instance" )
        self._url = url
        if not ( self._url == None ):
            self.setAttribute( "url", "#" + self._url )

class _LibraryElement( Element ):

    _libraryType = None
    _name = None
    _id = None
    
    def __init__( self, libraryType ):
        Element.__init__( self, "library" )
        self._libraryType = libraryType
        self.setAttribute( "type", CP.LT.str[ self._libraryType ] )
    

class _AnimationElement( Element ):
    pass

class _ChannelElement( Element ):
    pass

class _SamplerElement( Element ):
    pass

class _ControllerElement( Element ):
    pass

class _SkinElement( Element ):
    pass

class _CombinerElement( Element ):
    pass

class _JointsElement( Element ):
    pass

class _GeometryElement( Element ):
    
    _name = ""
    _id = ""
    
    def __init__( self, id, name ):
        Element.__init__( self, "geometry" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )

class _MeshElement( Element ):
    
    _name = ""
    _id = ""
    
    def __init__( self, id, name ):
        Element.__init__( self, "mesh" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )

class _LinesElement( Element ):
    pass

class _LinestripsElement( Element ):
    pass

class _PolygonsElement( Element ):
    
    _count = None
    _material = None
    
    def __init__( self, count, material ):
        Element.__init__( self, "polygons" )
        self._count = count        
        self._material = material        
        
        if not ( self._count == None ):
            self.setAttribute( "count", "%i" % self._count )
        if not ( self._material == None ):
            self.setAttribute( "material", "#" + self._material )

class _PElement( Element, Childless ):
    
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def __init__( self ):
        Element.__init__( self, "p" )
    
    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )    

class _TrianglesElement( Element ):
    pass

class _TrifansElement( Element ):
    pass

class _TristripsElement( Element ):
    pass

class _VerticesElement( Element ):
    
    _id = None
    _name = None
    _count = None
    
    def __init__( self, id, name, count ):
        Element.__init__( self, "vertices" )
        self._id = id
        self._name = name
        self._count = count        
        
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "name", self._name )
        if not ( self._count == None ):
            self.setAttribute( "count", "%i" % self._count )


class _SourceElement( Element ):
    
    _id = None
    _name = None
    
    def __init__( self, id, name ):
        Element.__init__( self, "source" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )

class _AccessorElement( Element ):
    
    _count = None
    _id = None
    _offset = None
    _source = None
    _stride = None
    
    def __init__( self, count, id, offset, source, stride ):
        Element.__init__( self, "accessor" )
        self._count = count
        self._id = id
        self._offset = offset
        self._source = source
        self._stride = stride
        if not ( self._count == None ):
            self.setAttribute( "count", "%i" % self._count )
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )        
        if not ( self._offset == None ):
            self.setAttribute( "offset", "%i" % self._offset )
        if not ( self._source == None ):
            self.setAttribute( "source", "#" + self._source )
        if not ( self._stride == None ):
            self.setAttribute( "stride", "%i" % self._stride )


class _ArrayElement( Element ):
    pass

class _BoolArrayElement( Element ):
    pass

class _FloatArrayElement( Element ):
    
    _count = None
    _id = None
    _name = None    
    _digits = None
    _magnitude = None
    _data = None
    _dataType = 'STRING'
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def setDataType( self, dataType ):
        self._dataType = dataType
    
    def setCount( self, count ):
        self._count = count
        self.setAttribute( "count", "%i" % self._count )
    
    def __init__( self, count, id, name, digits, magnitude ):
        Element.__init__( self, "float_array" )
        self._count = count        
        self._id = id
        self._name = name
        self._digits = digits
        self._magnitude = magnitude
        if not ( self._count == None ):
            self.setAttribute( "count", "%i" % self._count )
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "name", self._name )
        if not ( self._digits == None ):
            self.setAttribute( "digits", "%i" % self._digits )
        if not ( self._magnitude == None ):
            self.setAttribute( "magnitude", "%i" % self._magnitude )

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        
        global niceFormat        
        
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            
            
            # writer.write(">%s%s</%s>%s" % ( self._data, newl + indent, self.tagName, newl ) )
            writer.write( ">" )
            dataString = ''
            if ( self._dataType == 'POSITION' ):
                for v in self._data:
                    dataString = "\n%.6f %.6f %.6f" % ( v.co[0], v.co[1], v.co[2] )
                    writer.write( dataString )
            elif ( self._dataType == 'NORMAL' ):
                for n in self._data:
                    dataString = "\n%.6f %.6f %.6f" % ( n[0], n[1], n[2] )
                    writer.write( dataString )
            elif ( self._dataType == 'UV' ):
                for uv in self._data:
                    dataString = "\n%.6f %.6f" % ( uv[0], uv[1] )
                    writer.write( dataString )
            else:
                # Replace '\n' with '\n + indent + addindent'
                if ( niceFormat ):
                    replaceString = '\n' + indent + addindent
                    self._data = self._data.replace( '\n', replaceString )
                _write_data( writer, self._data )
            writer.write("%s</%s>%s" % ( newl + indent, self.tagName, newl ) )


class _IntArrayElement( Element ):
    pass

class _NameArrayElement( Element ):
    pass

class _InputElement( Element ):
    
    _idx = None
    _semantic = None
    _source = None
    
    def __init__( self, idx, semantic, source ):
        Element.__init__( self, "input" )
        self._idx = idx
        self._semantic = semantic
        self._source = source
        if not ( self._idx == None ):
            self.setAttribute( "idx", "%i" % (self._idx) )
        if not ( self._semantic == None ):
            self.setAttribute( "semantic", CP.IS.str[ self._semantic ] )
        if not ( self._source == None ):
            self.setAttribute( "source", "#" + self._source )

class _MaterialElement( Element ):
    
    _id = None
    _name = None
    
    def __init__( self, id, name ):
        Element.__init__( self, "material" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", id )
        if not ( self._name == None ):
            self.setAttribute( "name", name )
        

class _ShaderElement( Element ):
    _id = None
    _name = None
    
    def __init__( self, id, name ):
        Element.__init__( self, "shader" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "id", self._name )

class _PassElement( Element ):
    def __init__( self ):
        Element.__init__( self, "pass" )

class _TechniqueElement( Element ):
    
    _profile = None
    
    def setProfile( self, profile ):
        self._profile = profile
    
    def __init__( self, profile ):
        Element.__init__( self, "technique" )
        self._profile = profile
        if not ( self._profile == None ):
            self.setAttribute( "profile", CP.str[ self._profile ] )

class _ImageElement( Element ):
    
    _id = None
    _name = None
    _height = None
    _width = None
    _depth = None
    _source = None
    _format = None
    
    def setIdAndName( self, arg ):
        _id = arg
        _name = arg
        self.setAttribute( "id", _id )
        self.setAttribute( "name", _name )
    
    def setWidth( self, width ):
        self._width = width
        self.setAttribute( "width", "%i" % (self._width) )
    
    def setHeight( self, height ):
        self._height = height
        self.setAttribute( "height", "%i" % (self._height) )
    
    def setSource( self, source ):
        self._source = source
        self.setAttribute( "source", self._source )

    def __init__( self, id, name, height, width, depth,
                        source, format ):
        Element.__init__( self, "image" )
        
        self._id = id
        self._name = name
        self._height = height
        self._width = width
        self._depth = depth
        self._source = source
        self._format = format
        
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "name", self._name )
        if not ( self._height == None ):
            self.setAttribute( "height", "%i" % self._height )
        if not ( self._width == None ):
            self.setAttribute( "width", "%i" % self._width )
        if not ( self._depth == None ):
            self.setAttribute( "depth", "%i" % self._depth )
        if not ( self._source == None ):
            self.setAttribute( "source", self._source )
        if not ( self._format == None ):
            self.setAttribute( "format", self._format )

class _LightElement( Element ):
    
    _id = None
    _name = None
    _type = None
    
    def __init__( self, id, name, type ):
        Element.__init__( self, "light" )
        self._id = id
        self._name = name
        self._type = type
        
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "name", self._name )
        if not ( self._type == None ):
            self.setAttribute( "type", "%s" % CP.LIGHT.str[ self._type ] )

class _TextureElement( Element ):
    
    _id = None
    _name = None
    
    def __init__( self, id, name ):
        Element.__init__( self, "texture" )
        self._id = id
        self._name = name
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "name", self._name )

class _ProgramElement( Element ):
    
    _id = None
    _name = None
    _url = None
    
    def __init__( self, id, name, url ):
        Element.__init__( self, "program" )
        self._id = id
        self._name = name
        self._url = url
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )
        if not ( self._name == None ):
            self.setAttribute( "name", self._name )
        if not ( self._url == None ):
            self.setAttribute( "url", self._url ) 

class _CodeElement( Element ):
    pass

class _EntryElement( Element ):
    pass

class _ParamElement( Element, Childless ):
    
    _id = None
    _name = None
    _type = None
    _flow = None
    _semantic = None
    _sid = None
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def __init__( self, id, name, type, flow, semantic, sid ):
        Element.__init__( self, "param" )
        self._id = id
        self._name = name
        self._type = type
        self._flow = flow
        self._semantic = semantic
        self._sid = sid
        if not ( self._id == None ):
            self.setAttribute( "id", self._id )        
        if not ( self._name == None ):
            self.setAttribute( "name", CP.PN.str[ self._name ] )
        if not ( self._type == None ):
            self.setAttribute( "type", self._type )
        if not ( self._flow == None ):
            self.setAttribute( "flow", CP.FT.str[ self._flow ] )

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )

class _MatrixElement( Element, Childless ):
    
    _sid = None
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def __init__( self, sid ):
        Element.__init__( self, "matrix" )
        self._sid = sid
        if not ( self._sid == None ):
            self.setAttribute( "sid", self._sid )

# Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            # Replace '\n' with '\n + indent + addindent'
            replaceString = '\n' + indent + addindent
            self._data = self._data.replace( '\n', replaceString )
            writer.write(">%s%s</%s>%s" % ( self._data, newl + indent, self.tagName, newl ) )

class _LookAtElement( Element ):
    pass

class _PerspectiveElement( Element ):
    pass

class _RotateElement( Element, Childless ):
    
    _sid = None
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def __init__( self, sid ):
        Element.__init__( self, "rotate" )
        self._sid = sid
        if not ( self._sid == None ):
            self.setAttribute( "sid", self._sid )

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )    

class _ScaleElement( Element, Childless ):
    
    _sid = None
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def __init__( self, sid ):
        Element.__init__( self, "scale" )
        self._sid = sid
        if not ( self._sid == None ):
            self.setAttribute( "sid", self._sid )

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )

class _SkewElement( Element ):
    pass

class _TranslateElement( Element, Childless ):
    
    _sid = None
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self, data ):
        return self._data
    
    def __init__( self, sid ):
        Element.__init__( self, "translate" )
        self._sid = sid
        if not ( self._sid == None ):
            self.setAttribute( "sid", self._sid )

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )    
    

class _AuthorElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "author" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )  

class _AuthoringToolElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "authoring_tool" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )  

class _CreatedElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "created" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )  

class _ModifiedElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "modified" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )  

class _RevisionElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "modified" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )  

class _SourceDataElement( Element, Childless ):
    
    _data = None
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "source_data" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            
            # Fast data write
            # writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) )
            writer.write( ">" )
            _write_data( writer, self._data )
            writer.write( "</%s>%s" % (self.tagName, newl ) )

class _CopyrightElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "copyright" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 

class _TitleElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "title" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 


class _SubjectElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "subject" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 

class _KeywordsElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "keywords" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 

class _CommentsElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "comments" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 

class _UnitElement( Element, Childless ):
    
    _name = None
    _meter = None
    
    def __init__( self, name, meter ):
        Element.__init__( self, "unit" )
        
        self._name = name
        self._meter = meter
        
        if not ( self._id == None ):
            self.setAttribute( "name", self._name )
        if not ( self._name == None ):
            self.setAttribute( "meter", self._meter )

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 

class _UpAxisElement( Element, Childless ):
    
    _data = ""
    
    def setData( self, data ):
        self._data = data
    
    def getData( self ):
        return self._data

    def __init__( self, data ):
        Element.__init__( self, "up_axis" )
        if not ( data == None ):
            self._data = data

    # Overloaded writexml function so we can keep data within one line
    # This implementation was taken from the Python 2.3 minidom impl.
    # of class Element
    def writexml( self, writer, indent="", addindent="", newl="" ):
        writer.write(indent+"<" + self.tagName)

        attrs = self._get_attributes()
        a_names = attrs.keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" %s=\"" % a_name)
            _write_data( writer, attrs[a_name].value)
            writer.write("\"")
        if ( self._data == None ):
            writer.write( "/>%s" % newl )
        else:
            writer.write(">%s</%s>%s" % (self._data, self.tagName, newl ) ) 

class _AssetElement( Element ):

    def __init__( self ):
        Element.__init__( self, "asset" )



class _ExtraElement( Element ):
    pass


# ----------------------------------------------
#    Utility functions
# ----------------------------------------------
def toAngle( radians ):
    return ( radians * 180 ) / 3.14159265

# ----------------------------------------------
#    BLENDER to COLLADA helper functions
# ----------------------------------------------

# --- Write mesh information to dae file ---
def writeLight( libraryElement, light ):
    
    lightName = light.getName( )
    lightID = lightName + "-Lib"
    lightType = light.getType( )
    lightFlags = light.getMode( )
    distance = light.getDist( )
    energy = light.getEnergy( )
    quad1 = light.getQuad1( )
    quad2 = light.getQuad2( )

    # Blender LAMP type --> Collada POINT type
    if ( lightType == 0 ):
        lightElement = dae.createLightElement( lightID, lightName, CP.LIGHT.POINT )
        colorParam = dae.createParamElement( None, CP.PN.COLOR, "float3", CP.FT.IN, None, None )
        colorParam.setData( "%.6f %.6f %.6f" % ( light.col[ 0 ], light.col[ 1 ], light.col[ 2 ]  ) )
        attenuationParam = dae.createParamElement( None, CP.PN.ATTENUATION, "token", CP.FT.IN, None, None )
        attenuationScaleParam = dae.createParamElement( None, CP.PN.ATTENUATION_SCALE, "float", CP.FT.IN, None, None )
        
        if ( lightFlags & light.Modes[ 'Quad' ] ):
            attenuationScale = 2.0 / ( quad2 * distance * energy )
            attenuationParam.setData( "QUADRATIC" )
            attenuationScaleParam.setData( "%.6f" % ( attenuationScale ) )
        else:
            attenuationScale = 2.0 / ( distance * energy )
            attenuationParam.setData( "LINEAR" )
            attenuationScaleParam.setData( "%.6f" % ( attenuationScale ) )
        
        lightElement.appendChild( colorParam )
        lightElement.appendChild( attenuationParam )
        lightElement.appendChild( attenuationScaleParam )
        libraryElement.appendChild( lightElement )
    
    # Blender SUN type --> Collada DIRECTIONAL type
    elif ( lightType == 1 ):
        lightElement = dae.createLightElement( lightID, lightName, CP.LIGHT.DIRECTIONAL )
        colorParam = dae.createParamElement( None, CP.PN.COLOR, "float3", CP.FT.IN, None, None )
        colorParam.setData( "%.6f %.6f %.6f" % ( light.col[ 0 ], light.col[ 1 ], light.col[ 2 ]  ) )
                
        lightElement.appendChild( colorParam )
        libraryElement.appendChild( lightElement )
    
    # Blender SPOT type --> Collada SPOT type
    elif ( lightType == 2 ):
        lightElement = dae.createLightElement( lightID, lightName, CP.LIGHT.SPOT )
        colorParam = dae.createParamElement( None, CP.PN.COLOR, "float3", CP.FT.IN, None, None )
        colorParam.setData( "%.6f %.6f %.6f" % ( light.col[ 0 ], light.col[ 1 ], light.col[ 2 ]  ) )
        
        attenuationParam = dae.createParamElement( None, CP.PN.ATTENUATION, "token", CP.FT.IN, None, None )
        attenuationScaleParam = dae.createParamElement( None, CP.PN.ATTENUATION_SCALE, "float", CP.FT.IN, None, None )
        
        # Is this the correct attenuation algorithm?
        if ( lightFlags & light.Modes[ 'Quad' ] ):
            attenuationScale = 2.0 / ( quad2 * distance * energy )
            attenuationParam.setData( "QUADRATIC" )
            attenuationScaleParam.setData( "%.6f" % ( attenuationScale ) )
        else:
            attenuationScale = 2.0 / ( distance * energy )
            attenuationParam.setData( "LINEAR" )
            attenuationScaleParam.setData( "%.6f" % ( attenuationScale ) )
        
        # Export ANGLE, FALLOFF and FALLOFF_SCALE
        angle = light.getSpotSize( )
        angleParam = dae.createParamElement( None, CP.PN.ANGLE, "float", CP.FT.IN, None, None )
        angleParam.setData( "%.6f" % angle )
        
        
        falloff = light.getSpotSize( )
        falloffParam = dae.createParamElement( None, CP.PN.FALLOFF, "token", CP.FT.IN, None, None )
        falloffParam.setData( "LINEAR" )        
        
        falloffScale = light.getSpotBlend( ) * 128.0
        falloffScaleParam = dae.createParamElement( None, CP.PN.FALLOFF_SCALE, "float", CP.FT.IN, None, None )
        falloffScaleParam.setData( "%.6f" % falloffScale )        
        
        lightElement.appendChild( colorParam )
        lightElement.appendChild( attenuationParam )
        lightElement.appendChild( attenuationScaleParam )
        lightElement.appendChild( angleParam )
        lightElement.appendChild( falloffParam )
        lightElement.appendChild( falloffScaleParam )
        libraryElement.appendChild( lightElement )        
    
    elif ( lightType == 3 ):
        lightElement = dae.createLightElement( lightID, lightName, CP.LIGHT.AMBIENT )
        colorParam = dae.createParamElement( None, CP.PN.COLOR, "float3", CP.FT.IN, None, None )
        colorParam.setData( "%.6f %.6f %.6f" % ( light.col[ 0 ], light.col[ 1 ], light.col[ 2 ]  ) )
        lightElement.appendChild( colorParam )
        libraryElement.appendChild( lightElement )
    elif ( lightType == 4 ):
        print "Warning! Export of light type Area not supported! Light will not export."
    elif ( lightType == 5 ):
        print "Warning! Export of light type Photon not supported! Light will not export."


# --- Write mesh information to dae file ---
def writeMesh( libraryElement, mesh ):
    
    meshName = mesh.name
    meshID = mesh.name + "-Lib"
    hasFaceUV = mesh.hasFaceUV()
    nrVerts = len( mesh.verts )
    nrFaces = len( mesh.faces )    
    geometry = dae.createGeometryElement( meshID, meshName )
    meshElement = dae.createMeshElement( )
    posID = meshName + "-Pos"
    normalID = meshName + "-Normal"
    uvID = meshName + "-UV1"
    position = dae.createSourceElement( posID, posID )
    normal = dae.createSourceElement( normalID, normalID )
    uv = dae.createSourceElement( uvID, uvID )
    posArrayID = posID + "-Array"
    noArrayID = normalID + "-Array"
    uvArrayID = uvID + "-Array"
    floatArrayPos = dae.createFloatArrayElement( nrVerts * 3, posArrayID, posArrayID )
    floatArrayNo = dae.createFloatArrayElement( -1, noArrayID, noArrayID )
    floatArrayUV = dae.createFloatArrayElement( -1, uvArrayID, uvArrayID )
    

    # Set elements data as list data for faster output!
    floatArrayPos.setDataType( 'POSITION' )
    floatArrayNo.setDataType( 'NORMAL' )
    floatArrayUV.setDataType( 'UV' )
    
    # Output nice format for vertex data
    # TODO: Make this routine better
    # startTime = sys.time()
    # posDataArray = array('f')
    # posData = ""
##    for v in mesh.verts:
##        #posDataArray.append( v.co[ 0 ] )
##        #posDataArray.append( v.co[ 1 ] )
##        #posDataArray.append( v.co[ 2 ] )
##        posData += "\n%.6f %.6f %.6f" % ( v.co[0], v.co[1], v.co[2] )
    
    posData = mesh.verts
    
    # Get normal and UV data
    #no = ""
    no = []
    #st = ""
    st = []
    nrExclusiveVertices = 0
    nrUVCoords = 0
    faceNr = 0
    for f in mesh.faces:
        nrVert = len( f.v )
        if ( (nrVert == 3) or (nrVert == 4 ) ):
            if ( f.smooth != 0 ):        # If smooth than fetch normal for every vertex in face
                for v2 in f.v:
                    #no += "\n%.6f %.6f %.6f" % ( v2.no[0], v2.no[1], v2.no[2] )
                    no.append( v2.no )
                    nrExclusiveVertices += 1
            elif ( f.smooth == 0 ):
                # FIXME: There is unknown bug here...
                for i in f.v:
                    if ( len( f.no ) > 0 ):
                        #no += "\n%.6f %.6f %.6f" % ( f.no[0], f.no[1], f.no[2] )
                        no.append( f.no )
                    else:
                        print "Warning! Face has nao normal. Outputing (0,0,0) as normal instead!"
                        # TODO: Calculate normal instead!
                        #no += "\n%.6f %.6f %.6f" % ( 0.0, 0.0, 0.0 )
                        no.append( [ 0.0, 0.0, 0.0 ] )
                    nrExclusiveVertices += 1
            
            if ( hasFaceUV ):    # Mesh has face UV set so let's export UV coordinates
                for i in range( nrVert ):
                    #st += "\n%.6f %.6f" % ( f.uv[i][0], f.uv[i][1] )
                    st.append( f.uv[i] )
                    nrUVCoords += 1
        else:
            print "Warning: Face in %s" % meshName + " is not a triangle or quad! Face ignored."
    # endTime = sys.time( )
    # print "Getting mesh data time: %.6f" % ( endTime - startTime )
    
    floatArrayNo.setCount( nrExclusiveVertices * 3 )
    floatArrayNo.setData( no )
    floatArrayPos.setData( posData )
    #floatArrayPos.setData( posDataArray.tostring( ) )
    normal.appendChild( floatArrayNo )
    position.appendChild( floatArrayPos )
    
    if ( hasFaceUV ):
        floatArrayUV.setCount( nrUVCoords * 2 )
        floatArrayUV.setData( st )
        uv.appendChild( floatArrayUV )
    
    meshElement.appendChild( position )
    meshElement.appendChild( normal )
   

    # Write COMMON technique elements
    techniquePos = dae.createTechniqueElement( CP.COMMON )
    techniqueNormal = dae.createTechniqueElement( CP.COMMON )
    accessorPos = dae.createAccessorElement( nrExclusiveVertices, posArrayID, posArrayID + "-Accessor", 0, 3 )
    accessorNormal = dae.createAccessorElement( nrExclusiveVertices, noArrayID, noArrayID + "-Accessor", 0 , 3 )
    
    paramPosX = dae.createParamElement( None, CP.PN.X, "float", None )
    paramPosY = dae.createParamElement( None, CP.PN.Y, "float", None )
    paramPosZ = dae.createParamElement( None, CP.PN.Z, "float", None )
    
    paramNoX = dae.createParamElement( None, CP.PN.X, "float", None )
    paramNoY = dae.createParamElement( None, CP.PN.Y, "float", None )
    paramNoZ = dae.createParamElement( None, CP.PN.Z, "float", None )

    accessorPos.appendChild( paramPosX )
    accessorPos.appendChild( paramPosY )
    accessorPos.appendChild( paramPosZ )

    accessorNormal.appendChild( paramNoX )
    accessorNormal.appendChild( paramNoY )
    accessorNormal.appendChild( paramNoZ )


    techniquePos.appendChild( accessorPos )
    position.appendChild( techniquePos )
    techniqueNormal.appendChild( accessorNormal )
    normal.appendChild( techniqueNormal )

    if ( hasFaceUV ):
        meshElement.appendChild( uv )
        techniqueUV = dae.createTechniqueElement( CP.COMMON )
        accessorUV = dae.createAccessorElement( nrUVCoords, uvArrayID, uvArrayID + "-Accessor", 0, 2 )
        
        paramS = dae.createParamElement( None, CP.PN.S, "float", None )
        paramT = dae.createParamElement( None, CP.PN.T, "float", None )
        
        accessorUV.appendChild( paramS )
        accessorUV.appendChild( paramT )
        
        techniqueUV.appendChild( accessorUV )
        uv.appendChild( techniqueUV )


    # Write vertices and polygons information
    verticesName = meshName + "-Vertices"
    vertices = dae.createVerticesElement( verticesName, verticesName, nrExclusiveVertices )
    vInput = dae.createInputElement( None, CP.IS.POSITION, posID )
    
    # TODO: Write UV here if we have sticky vertices. Also write normal data
    # if the whole mesh is smooth
    
    # Write polygon element
    materials = mesh.getMaterials( 1 )
    
    if ( len( materials ) > 1 ):
        print "Warning! Mesh has multiple materials. Will only export the first one"
    
    # TODO: Write polygon element for each material
    material = None
    polygons = None
    if len( materials ) > 0:
        material = materials[ 0 ]
        if ( material != None ):
            materialSource = material.getName( ) + "-Lib"
            polygons = dae.createPolygonsElement( nrFaces, materialSource )
        else:
            polygons = dae.createPolygonsElement( nrFaces )
    else:
        polygons = dae.createPolygonsElement( nrFaces )
    
    # Write input semantics for polygons
    vertexInput = dae.createInputElement( 0, CP.IS.VERTEX, verticesName )
    normalInput = dae.createInputElement( 1, CP.IS.NORMAL, normalID )
    polygons.appendChild( vertexInput )
    polygons.appendChild( normalInput )
    
    if ( hasFaceUV ):
        uvInput = dae.createInputElement( 2, CP.IS.TEXCOORD, uvID )
        polygons.appendChild( uvInput )
    
    
    # Write <p> elements
        # NOTE: index here points to normal and (if present) texcoord index in floatarray
    index = -1
    for f in mesh.faces:
        pData = ""
        nrVertices = len( f.v )
        if ( (nrVertices == 3) or (nrVertices == 4 ) ):
            for v in f.v:
                index += 1
                if ( hasFaceUV ):
                    pData = pData + "%i %i %i" % ( v.index, index, index ) + " "
                else:
                    pData = pData + "%i %i" % ( v.index, index ) + " "
                
            pData = pData.strip()
            p = dae.createPElement( )
            p.setData( pData )
            polygons.appendChild( p )
        
    
    vertices.appendChild( vInput )
    meshElement.appendChild( vertices )
    meshElement.appendChild( polygons )
    geometry.appendChild( meshElement )
    libraryElement.appendChild( geometry )

# Blender to OpenGL materials
# With help from Nathan (Waffler at Elysiun)
def writeMaterial( libraryElement, material ):
    name = material.getName( )
    id = name + "-Lib"
    materialElement = dae.createMaterialElement( id, name )
    
    # Create common phong shader with one phong pass
    shaderElement = dae.createShaderElement( None, name + "-Phong" )
    technique = dae.createTechniqueElement( CP.COMMON )
    
    # Get material data
    col = material.getRGBCol( )
    speccol = material.getSpecCol( )
    spec = material.getSpec( ) * 0.5
    ambient = material.getAmb( )
    emit = material.getEmit( )
    transparency = 1.0 - material.getAlpha( )
    ref = material.getRef( )
    refcol = material.getMirCol()
    shininess = material.getHardness() / 4.0
    # refID = material.getIOR()

    
    mtextures = material.getTextures( )
    passElement = dae.createPassElement( )               

    # TODO: Perhaps add an option to split texture channels into
    # different <pass> elements instead

    # Input semantics for texture channel(s)            
    for mtex in mtextures:
        if not ( mtex == None ):
            texture = mtex.tex
            if ( texture.type == Texture.Types.IMAGE ):
                texSource = texture.getName() + "-Lib"
                input = dae.createInputElement( None, CP.IS.TEXTURE, texSource )
                passElement.appendChild( input )
                
    
    # Appereantly I cannot access the shaders so ... output
    # PHONG for now

    phongProgram = dae.createProgramElement( None, None,
                        CP.PIAU.str[ CP.PIAU.PHONG ] )
    
    # PHONG PARAMS
    colorParam = dae.createParamElement( None, CP.PN.COLOR, "float3", CP.FT.IN )
    colorParam.setData( "%.6f %.6f %.6f" % ( col[0], col[1], col[2] ) )
    ambientParam = dae.createParamElement( None, CP.PN.AMBIENT, "float3", CP.FT.IN )
    ambientParam.setData( "%.6f %.6f %.6f" % ( ambient * col[0], ambient * col[1], ambient * col[2] ) )                        
    diffuseParam = dae.createParamElement( None, CP.PN.DIFFUSE, "float3", CP.FT.IN )
    diffuseParam.setData( "%.6f %.6f %.6f" % ( col[0], col[1], col[2] ) )
    #transparencyParam = dae.createParamElement( None, CP.PN.TRANSPARENCY, "float", CP.FT.IN )
    #transparencyParam.setData( "%.6f" % transparency )                        
    specularParam = dae.createParamElement( None, CP.PN.SPECULAR, "float3", CP.FT.IN )
    specularParam.setData( "%.6f %.6f %.6f" % ( speccol[0] * spec, speccol[1]  * spec, speccol[2]  * spec ) )
    emissionParam = dae.createParamElement( None, CP.PN.EMISSION, "float3", CP.FT.IN )
    emissionParam.setData( "%.6f %.6f %.6f" % ( emit * col[0], emit * col[1], emit * col[2] ) )

    shininessParam = dae.createParamElement( None, CP.PN.SHININESS, "float", CP.FT.IN )
    shininessParam.setData( "%.6f" % ( shininess ) )

    # Output color as transparent color for now
    transparentParam = dae.createParamElement( None, CP.PN.TRANSPARENT, "float", CP.FT.IN )
    transparentParam.setData( "%.6f %.6f %.6f" % ( col[0], col[1], col[2]  ) )                
    transparencyParam = dae.createParamElement( None, CP.PN.TRANSPARENCY, "float", CP.FT.IN )
    transparencyParam.setData( "%.6f" % ( transparency ) )

    
    reflectParam = dae.createParamElement( None, CP.PN.REFLECTIVE, "float3", CP.FT.IN )
    reflectParam.setData( "%.6f %.6f %.6f" % ( refcol[0], refcol[1], refcol[2]  ) )                
    reflectivityParam = dae.createParamElement( None, CP.PN.REFLECTIVITY, "float", CP.FT.IN )
    reflectivityParam.setData( "%.6f" % ( ref  ) )
    
    # refractiveParam = dae.createParamElement( None, CP.PN.REFRACTIVEINDEX, "float3", CP.FT.IN )
    # refractiveParam.setData( "%.6f %.6f %.6f" % ( refID[0], refID[1], refID[2]  ) )                
    
    # TODO: Output SHININESS here. Perhaps calc from
    # Specular HARDNESS and SPECULARITY (?)
    phongProgram.appendChild( colorParam )
    phongProgram.appendChild( diffuseParam )
    phongProgram.appendChild( ambientParam )                        
    phongProgram.appendChild( transparencyParam )
    phongProgram.appendChild( specularParam )
    phongProgram.appendChild( emissionParam )
    phongProgram.appendChild( shininessParam )
    phongProgram.appendChild( transparentParam ) 
    phongProgram.appendChild( transparencyParam )                
    phongProgram.appendChild( reflectParam )                                   
    phongProgram.appendChild( reflectivityParam )
    # phongProgram.appendChild( refractiveParam )
    passElement.appendChild( phongProgram )
    technique.appendChild( passElement )
    
    
    shaderElement.appendChild( technique )
    materialElement.appendChild( shaderElement )            
    libraryElement.appendChild( materialElement )

# --- Write Image information to dae file ---
def writeImage( libraryElement, image ):
    name = image.getName( )
    id = name + "-Lib"
    imageElement = dae.createImageElement( id, name )

    # Getting the image size seems to generate 
    # python errors sometimes (for some reason)...
    # imageElement.setWidth( image.size[0] )
    # imageElement.setHeight( image.size[1] )
    filename = sys.expandpath( image.getFilename( ) )
    filename = filename.replace( "//", "/" )    
    filename = filename.replace( sys.sep, "/" )
    filename = "file://" + filename
    imageElement.setSource( filename )
    libraryElement.appendChild( imageElement )
    


def writeTexture( libraryElement, texture ):
    name = texture.getName( )
    id = name + "-Lib"
    
    if ( texture.getType() == "Image" ):
        # Create DIFFUSE texture element
        texID = name + "-Lib"
        textureElement = dae.createTextureElement( texID, name )
    
        # TODO: Depending on type here create different params and techniques
        textureParamElement = dae.createParamElement( None, CP.PN.DIFFUSE, "float3", CP.FT.OUT )
        textureElement.appendChild( textureParamElement )
    
        # Create COMMON Technique
        technique = dae.createTechniqueElement( CP.COMMON )
        textureElement.appendChild( technique )
    
        source = texture.getImage( )
        if not ( source == None ):
            sourceName = source.getName( ) + "-Lib"
            input = dae.createInputElement( None, CP.IS.IMAGE, sourceName )
            technique.appendChild( input )
    
        libraryElement.appendChild( textureElement )
    else:
        print "Warning! Texture is not of Image type. Texture not exported!"

# Write camera information to dae file
def writeCamera( libraryElement, camera ):
    
    cameraName = camera.getName()
    cameraID = cameraName + "-Lib" 
    cameraType = camera.getType()
    cameraElement = dae.createCameraElement( cameraID, cameraName )
    technique = dae.createTechniqueElement( CP.COMMON )
    optics = dae.createOpticsElement( )
    
    # Create PERSPECTIVE camera or ORTHO camera
    program = None
    
    # PERSPECTIVE Camera
    # print cameraType
    if ( cameraType == 0 ):
        program = dae.createProgramElement( None, None, CP.PIAU.str[ CP.PIAU.PERSPECTIVE ] )
        paramYFOV = dae.createParamElement( None, CP.PN.YFOV, "float", CP.FT.IN, None, None )
        #paramXFOV = dae.createParamElement( None, CP.PN.XFOV, "float", CP.FT.IN, None, None )
        
        # Calculate Y-FOV based on LENS value
        factor = 1.0
        lens = camera.getLens( )
        yfov = 90.0 * 3.1415926 * ( math.atan( (16.0 * factor) / lens ) )
        
        paramYFOV.setData( "%.6f" % yfov )
        program.appendChild( paramYFOV )
        
        #paramXFOV.setData( "%.6f" % xfov )
        #program.appendChild( paramXFOV )
        
    else:
        print "Warning! Orthographic cameras not supported. Camera not exported! "
        return
        #program = dae.createProgramElement( None, CP.PIAU.str[ CP.PIAU.ORTHOGRAPHIC ], None )
    
    paramZNear = dae.createParamElement( None, CP.PN.ZNEAR, "float", CP.FT.IN, None, None )
    paramZFar = dae.createParamElement( None, CP.PN.ZFAR, "float", CP.FT.IN, None, None )
    znear = camera.getClipStart( )
    zfar = camera.getClipEnd( )
    paramZNear.setData( "%.6f" % znear )
    paramZFar.setData( "%.6f" % zfar )
    program.appendChild( paramZNear )
    program.appendChild( paramZFar )
    optics.appendChild( program )
    technique.appendChild( optics )
    cameraElement.appendChild( technique )
    libraryElement.appendChild( cameraElement )




# --- Write header information to dae file ---
def writeHeader( colladaElement ):
    
    # Write asset information
    asset = dae.createAssetElement( )
    
    author = dae.createAuthorElement( __author__ )
    authoringtooldata = "Blender " + "%i" % Blender.Get( 'version' ) + " COLLADA Exporter"
    authoringtool = dae.createAuthoringToolElement( authoringtooldata )
    
    # Can I change this in Blender?
    upaxisdata = "Z_UP"
    upaxis = dae.createUpAxisElement( upaxisdata )
    # createddata = date( 2005, 12, 21)
    # created = dae.createCreatedElement( createddata )

    asset.appendChild( author )
    asset.appendChild( authoringtool )
    asset.appendChild( upaxis )
    colladaElement.appendChild( asset )

# --- Write libraries to dae file ---
def writeLibrary( colladaElement ):

    
    imageLibraryElement = dae.createLibraryElement( CP.LT.IMAGE )
    textureLibraryElement = dae.createLibraryElement( CP.LT.TEXTURE )
    materialLibraryElement = dae.createLibraryElement( CP.LT.MATERIAL )
    geometryLibraryElement = dae.createLibraryElement( CP.LT.GEOMETRY )
    lightLibraryElement = dae.createLibraryElement( CP.LT.LIGHT )
    cameraLibraryElement = dae.createLibraryElement( CP.LT.CAMERA )
    
    meshes = dict()
    lights = dict()
    cameras = dict()
    
    for obj in objects:
        objType = obj.getType( )
        dataName = obj.getData( True )
        data = obj.getData( False )
        if ( objType == 'Mesh' ):
            meshes[ dataName ] = NMesh.GetRaw( dataName )
        elif ( objType == 'Lamp' ):
            lights[ dataName ] = data
        elif ( objType == 'Camera' ):
            cameras[ dataName ] = data
    
    #meshes = NMesh.GetNames()
    #lights = Lamp.Get()
    #cameras = Camera.Get( )

    #print meshes
    #print lights
    #print cameras
    
    
    # Get raw meshes
    materials = dict()
    nrMeshes = len( meshes )
    if ( nrMeshes > 0 ):
        progress = 0.0
        addProgress = 1.0 / nrMeshes
        Window.DrawProgressBar( 0.0, "Writing mesh data..." )
        for key in meshes:
            rawMesh = meshes[ key ]
            if ( rawMesh.users > 0 ):
                writeMesh( geometryLibraryElement, rawMesh )
                progress += addProgress
                Window.DrawProgressBar( progress, "Exporting geometry %.0f %%..." % ( progress * 100.0 ) )
                meshMaterials = rawMesh.getMaterials( -1 )
                for material in meshMaterials:
                    key = material.getName( )
                    materials[ key ] = material
    
    #print materials
    
    # Get materials
    textures = dict()
    nrMaterials = len( materials )
    if ( nrMaterials > 0 ):
        progress = 0.0
        addProgress = 1.0 / nrMaterials
        for key in materials:
            material = materials[ key ]
            writeMaterial( materialLibraryElement, material )
            progress += addProgress
            Window.DrawProgressBar( progress, "Exporting materials %.0f %%..." % ( progress * 100.0 ) )
            materialTextures = material.getTextures( )
            for texture in materialTextures:
                if not ( texture == None ):
                    if ( texture.tex.type == Texture.Types.IMAGE ):
                        key = texture.tex.getName( )
                        textures[ key ] = texture.tex
    
    #print textures
    
    # Write textures
    images = dict()
    nrTextures = len( textures )
    if ( nrTextures > 0 ):
        progress = 0.0
        addProgress = 1.0 / nrTextures
        for key in textures:
            texture = textures[ key ]
            writeTexture( textureLibraryElement, texture )
            progress += addProgress
            Window.DrawProgressBar( progress, "Exporting textures %.0f %%..." % ( progress * 100.0 ) )
            image = texture.getImage( )
            if ( image != None ):
                imageName = image.getName( )
                images[ imageName ] = image
   
    #print images
 
    # Write images
    nrImages = len( images )
    if ( nrImages > 0 ):
        progress = 0.0
        addProgress = 1.0 / nrImages
        for key in images:
            image = images[ key ]
            writeImage( imageLibraryElement, image )
            progress += addProgress
            Window.DrawProgressBar( progress, "Exporting images %.0f %%..." % ( progress * 100.0 ) )
    
    
    # Write lights
    nrLights = len( lights )
    if ( nrLights > 0 ):
        progress = 0.0
        addProgress = 1.0 / nrLights
        for key in lights:
            light = lights[ key ]
            writeLight( lightLibraryElement, light )
            progress += addProgress
            Window.DrawProgressBar( progress, "Exporting lights %.0f %%..." % ( progress * 100.0 ) )
    
    # Write cameras
    nrCameras = len( cameras )
    if ( nrCameras > 0 ):
        progress = 0.0
        addProgress = 1.0 / nrCameras
        for key in cameras:
            camera = cameras[ key ]
            writeCamera( cameraLibraryElement, camera )
            progress += addProgress
            Window.DrawProgressBar( progress, "Exporting cameras %.0f %%..." % ( progress * 100.0 ) )
    
    colladaElement.appendChild( imageLibraryElement )
    colladaElement.appendChild( textureLibraryElement )
    colladaElement.appendChild( materialLibraryElement )
    colladaElement.appendChild( geometryLibraryElement )
    colladaElement.appendChild( lightLibraryElement )
    colladaElement.appendChild( cameraLibraryElement )

# --- Write node to dae ---
def writeNode( object, parentNode ):

    global exportBakedTransform
    
    newNodeName = object.name
    # newNodeName = object.name + "-Node"
    newNode = dae.createNodeElement( newNodeName, newNodeName, CP.NT.NODE )

    #print "Creating %s" % newNodeName
        
    # Create data for this node here
    url = object.getData( True )
    if not ( url == None ):
        url += "-Lib"
        instance = dae.createInstanceElement( url )
        newNode.appendChild( instance )
        
    #sidTranslate = newNodeName + "-Translate"
    #sidRotateX = newNodeName + "-Rotate-X" 
    if ( exportBakedTransform ):
        matrixElement = dae.createMatrixElement( )
        matrix = object.getMatrix( 'localspace' )
        data = ""
        matrix.transpose()
        for i in range( 4 ):
            data += "\n%.6f %.6f %.6f %.6f" % ( matrix[ i ].x, matrix[ i ].y,
                                                matrix[ i ].z, matrix[ i ].w )
        matrixElement.setData( data )
        newNode.appendChild( matrixElement )
    else:
        translate = dae.createTranslateElement( )
        rotateX = dae.createRotateElement( )
        rotateY = dae.createRotateElement( )
        rotateZ = dae.createRotateElement( )
        scale = dae.createScaleElement( )
        location = object.getLocation()
        rotation = object.getEuler()
        size = object.getSize()
        data = "%.6f %.6f %.6f" % ( location[ 0 ], location[ 1 ], location[ 2 ] )
        dataX = "1 0 0 %.6f" % ( toAngle( rotation[ 0 ] ) )
        dataY = "0 1 0 %.6f" % ( toAngle( rotation[ 1 ] ) )
        dataZ = "0 0 1 %.6f" % ( toAngle( rotation[ 2 ] ) )
        dataScale = "%.6f %.6f %.6f" % ( size[ 0 ], size[ 1 ], size[ 2 ] )
        translate.setData( data )
        rotateX.setData( dataX )
        rotateY.setData( dataY )
        rotateZ.setData( dataZ )
        scale.setData( dataScale )
        newNode.appendChild( translate )
        newNode.appendChild( rotateX )
        newNode.appendChild( rotateY )    
        newNode.appendChild( rotateZ )
        newNode.appendChild( scale )
    
    # Check if anyone has Me as Parent
    returnNode = None
    for child in objects:
        parent = child.getParent()
        if ( object == parent ):
            returnNode = writeNode( child, newNode )
            if not ( returnNode == None ):
                newNode.appendChild( returnNode )

    parentNode.appendChild( newNode )
    
    return newNode

# --- Write scene to dae file ---
def writeScene( colladaElement ):
    
    # Write <scene> element
    currentScene = Scene.GetCurrent( )
    sceneName = currentScene.getName( )
    sceneElement = dae.createSceneElement( sceneName, sceneName )
    colladaElement.appendChild( sceneElement )
    
    
    # Write nodes
    if not ( objects == None ):
        nrObjects = len( objects )
        if ( nrObjects > 0 ):
            addProgress = 1.0 / nrObjects
            progress = 0.0
            Window.DrawProgressBar( 0.0, "Exporting Scene 0.0% ..." )
            for object in objects:
                if ( object.getParent() == None ):
                    writeNode( object, sceneElement )
                    progress += addProgress
                    Window.DrawProgressBar( progress, "Exporting Scene %.0f%% ..." % ( progress * 100.0 ) )
            Window.DrawProgressBar( 1.0, "Finished!" )

    # print objects

# ----------------------------------------------









# ----------------------------------------------
#    HELPER FUNCTIONS
# ----------------------------------------------

# --- Set id and name tag to element ---
#def setIdAndName( element, string ):
    
#    element.setAttribute( "id", string )
#    element.setAttribute( "name", string )
    
#    return

# --- Create COMMON Technique element ---
#def createCommonTechnique( ):
#    technique = dae.createElement( "technique" )    
#    technique.setAttribute( "profile", "COMMON" )
#
#    return technique

# --- Create Param element ---
#def createParamElement( name, type ):
#    param = dae.createElement( "param" )    
#    param.setAttribute( "name", name )
#    param.setAttribute( "type", type )
#
#    return param



# ----------------------------------------------
# --- MAIN ---
def main( filePath ):
    global dae
    global objects
    global exportSelected
    
    if not ( filePath == None ):
        
        print "Exporting: " + filePath + "..."

        startTime = sys.time()
            
        dae = COLLADADocument()
        dae.openFile( filePath )
        
        # Build node tree here
        collada = dae.createCOLLADAElement( )
        dae.appendChild( collada )
    
        if ( exportSelected == True ):
            objects = Object.GetSelected( )
            # print "Selected objects only!"
        else:
            objects = Object.Get( )    
    
        # Write header information
        writeHeader( collada )
        
        #Write library information
        writeLibrary( collada )
        
        # Write scene information
        writeScene( collada )
        
        dae.buildXML( )
        dae.cleanUp( )
        
        
        # Get scene objects
        # objects = Object.Get()
    
        endTime = sys.time()
        
        exportTime = endTime - startTime
        print "Export time: %.6f" % (exportTime )
        #print "</COLLADA Exporter v0.1>"

    # Window.DrawProgressBar( 1.0, "" )


def callback_fileselector( filename ):
    if ( sys.exists( filename ) == 1 ):
        overwrite = Draw.PupMenu( "File Already Exists, Overwrite?%t|Yes%x1|No%x0" )
        if ( overwrite == 1 ):
            main( filename )
    else:
        main( filename )


def ExportGUI( ):
    global exportSelected, exportBakedTransform
    
    # Window.DrawProgressBar( 0.0, ">>> COLLADA-" )    
    
    # print "<COLLADA Exporter v0.1>"
    
    # Export selected or all
    progressString = "COLLADA-"
    answer = Draw.PupMenu( "Export what?%t|All%x1|Selected only%x0" )
    if ( answer == 1 ):
        exportSelected = False
        progressString += "All-"
    elif ( answer == 0 ):
        exportSelected = True
        progressString +="Selected-"
    else:
        # print "</COLLADA Exporter v0.1>"
        # Window.DrawProgressBar( 1.0, "" )
        return
    
    # Window.DrawProgressBar( 0.0, progressString )
    answer2 = Draw.PupMenu( "Export as matrix element?%t|Yes%x1|No%x0" )
    if ( answer2 == 1 ):
        exportBakedTransform = True
        progressString +="Matrix"
    elif ( answer2 == 0 ):
        exportBakedTransform = False
        progressString +="Transform"
    else:
        # print "</COLLADA Exporter v0.1>"
        # Window.DrawProgressBar( 1.0, "" )
        return

    # Window.DrawProgressBar( 0.0, progressString )
    
    print "Export options: " + progressString
    
    defaultFileName = Blender.Get('filename') + ".dae"
    defaultFileName = defaultFileName.replace( '.blend', '' )
    Window.FileSelector( callback_fileselector, 'Export .dae', defaultFileName )
    



# Run the script
if not ( _ERROR == True ):
    ExportGUI()
    
