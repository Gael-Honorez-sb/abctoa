import site, os, time, re, sys, ctypes, json

sys.path.append("@ARNOLD_ROOT@/bin") 
site.addsitedir(r"@ARNOLD_ROOT@/python")
from arnold import *

AiBegin()

def GetParamValue(pentry, val, type):
  if type == AI_TYPE_BYTE:
     return int(val.contents.BYTE)
  elif type == AI_TYPE_INT:
     return int(val.contents.INT)
  elif type == AI_TYPE_UINT:
     return int(val.contents.UINT)
  elif type == AI_TYPE_BOOLEAN:
     return True if (val.contents.BOOL != 0) else False
  elif type == AI_TYPE_FLOAT:
     return float(val.contents.FLT)
  elif type == AI_TYPE_VECTOR:
     return [val.contents.VEC.x, val.contents.VEC.y, val.contents.VEC.z]
  elif type == AI_TYPE_VECTOR2:
     return [val.contents.VEC2.x, val.contents.VEC2.y]
  elif type == AI_TYPE_RGB:
     return [val.contents.RGB.r, val.contents.RGB.g, val.contents.RGB.b]
  elif type == AI_TYPE_RGBA:
     return [val.contents.RGBA.r, val.contents.RGBA.g, val.contents.RGBA.b, val.contents.RGBA.a]
  elif type == AI_TYPE_STRING:
     return str(val.contents.STR)
  elif type == AI_TYPE_POINTER:
     return "%p" % val.contents.PTR
  elif type == AI_TYPE_NODE:
     name = AiNodeGetName(val.contents.PTR)
     return str(name)
  elif type == AI_TYPE_ENUM:
    enums = {}
    enums["values"] = []
    enum = AiParamGetEnum(pentry)
    index = 0
    while True:
       value = AiEnumGetString(enum, index)
       index += 1
       if not value:
          break
       enums["values"].append(value)
    enums["default"] =  val.contents.INT

    return enums
  elif type == AI_TYPE_MATRIX:
     return None
  elif type == AI_TYPE_ARRAY:
     array = val.contents.ARRAY.contents
     nelems = AiArrayGetNumElements(array)
     if nelems == 0:
        return nelems
     elif nelems == 1:
        if AiArrayGetType(array) == AI_TYPE_FLOAT:
           return "%g" % AiArrayGetFlt(array, 0)
        elif AiArrayGetType(array) == AI_TYPE_VECTOR:
           vec = AiArrayGetVec(array, 0)
           return "%g, %g, %g" % (vec.x, vec.y, vec.z)
        elif AiArrayGetType(array) == AI_TYPE_VECTOR2:
           pnt2 = AiArrayGetVec2(array, 0)
           return "%g, %g, %g" % (pnt2.x, pnt2.y)
        elif AiArrayGetType(array) == AI_TYPE_RGB:
           rgb = AiArrayGetRGB(array, 0)
           return "%g, %g, %g" % (rgb.r, rgb.g, rgb.b)
        elif AiArrayGetType(array) == AI_TYPE_RGBA:
           rgba = AiArrayGetRGBA(array, 0)
           return "%g, %g, %g" % (rgba.r, rgba.g, rgba.b, rgba.a)
        elif AiArrayGetType(array) == AI_TYPE_POINTER:
           ptr = cast(AiArrayGetPtr(array, 0), POINTER(AtNode))
           return "%p" % ptr
        elif AiArrayGetType(array) == AI_TYPE_NODE:
           ptr = cast(AiArrayGetPtr(array, 0), POINTER(AtNode))
           name = AiNodeGetName(ptr)
           return str(name)
        else:
           return ""
     else:
        return nelems
  else:
     return ""


nodes = {}
i = 0
it = AiUniverseGetNodeEntryIterator(AI_NODE_ALL)
while not AiNodeEntryIteratorFinished(it):

  nentry   = AiNodeEntryIteratorGetNext(it)
  nodename = AiNodeEntryGetName(nentry)
  typename = AiNodeEntryGetTypeName(nentry)

  if typename != "shape" and typename != "light":
    continue

  nodes[nodename] = {}

  nodes[nodename]["type"] = typename
  nodes[nodename]["params"] = []

  node_entry = AiNodeEntryLookUp(nodename)
  num_params = AiNodeEntryGetNumParams(node_entry)
  for p in range(num_params):
    pentry = AiNodeEntryGetParameter(node_entry, p)

    param_type  = AiParamGetType(pentry)
    param_value = AiParamGetDefault(pentry)
    param_name  = AiParamGetName(pentry)
    if param_type == AI_TYPE_POINTER:
      continue
    
    param = {}
    param["name"] = param_name
    param["value"] = GetParamValue(pentry, param_value, param_type)
    param["type"] = param_type
    if param_name == "smoothing":
      param["value"] = not param["value"]
    nodes[nodename]["params"].append(param)



AiNodeEntryIteratorDestroy(it)

f = file(os.path.join("@CMAKE_SOURCE_DIR@","maya", "scripts","shaderManager","propertywidgets", "arnold_nodes.json"), "w")

json.dump(nodes, f, sort_keys=True, indent=4)
f.close()
AiEnd()