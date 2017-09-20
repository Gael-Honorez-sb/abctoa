# Alembic Holder
# Copyright (c) 2014, Gael Honorez, All rights reserved.
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3.0 of the License, or (at your option) any later version.
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
# You should have received a copy of the GNU Lesser General Public
# License along with this library.

from PySide2.QtGui import *
from PySide2.QtCore import *
from PySide2.QtWidgets import *
#from argu_common import *

from arnold import *

class PropertyWidget(QFrame):
   BYTE = 0
   INT = 1
   UINT = 2

   RGB = 0
   RGBA = 1

   VECTOR = 0
   POINT = 1
   POINT2 = 2

   propertyChanged = Signal(dict)

   def __init__(self, param, parent = None):
      QFrame.__init__(self, parent)

      self.setBackgroundRole(QPalette.Dark)
      self.setAutoFillBackground(True)
      self.setSizePolicy(QSizePolicy.Minimum, QSizePolicy.Fixed)

      layout = QHBoxLayout()

      self.title = QLabel(param["name"], self)
      layout.addWidget(self.title)
      layout.addStretch()

      self.setLayout(layout)

      self.expanded = False
      self.normalSize = QSize(self.width(), self.height())
      self.expandedSize = QSize(self.width(), self.height() + 100)

   def GetParamValueAsString(self, pentry, val, type):
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
      elif type == AI_TYPE_VECTOR or type == AI_TYPE_POINT:
         return "%g, %g, %g" % (val.contents.PNT.x, val.contents.PNT.y, val.contents.PNT.z)
      elif type == AI_TYPE_POINT2:
         return "%g, %g" % (val.contents.PNT.x, val.contents.PNT.y)
      elif type == AI_TYPE_RGB:
         return "%g, %g, %g" % (val.contents.RGB.r, val.contents.RGB.g, val.contents.RGB.b)
      elif type == AI_TYPE_RGBA:
         return "%g, %g, %g, %g" % (val.contents.RGBA.r, val.contents.RGBA.g, val.contents.RGBA.b, val.contents.RGBA.a)
      elif type == AI_TYPE_STRING:
         return val.contents.STR
      elif type == AI_TYPE_POINTER:
         return "%p" % val.contents.PTR
      elif type == AI_TYPE_NODE:
         name = AiNodeGetName(val.contents.PTR)
         return str(name)
      elif type == AI_TYPE_ENUM:
         #enum = AiParamGetEnum(pentry)
         return val.contents.INT#AiEnumGetString(enum, val.contents.INT)
      elif type == AI_TYPE_MATRIX:
         return ""
      elif type == AI_TYPE_ARRAY:
         array = val.contents.ARRAY.contents
         nelems = array.nelements
         if nelems == 0:
            return "(empty)"
         elif nelems == 1:
            if array.type == AI_TYPE_FLOAT:
               return "%g" % AiArrayGetFlt(array, 0)
            elif array.type == AI_TYPE_VECTOR:
               vec = AiArrayGetVec(array, 0)
               return "%g, %g, %g" % (vec.x, vec.y, vec.z)
            elif array.type == AI_TYPE_POINT:
               pnt = AiArrayGetPnt(array, 0)
               return "%g, %g, %g" % (pnt.x, pnt.y, pnt.z)
            elif array.type == AI_TYPE_RGB:
               rgb = AiArrayGetRGB(array, 0)
               return "%g, %g, %g" % (rgb.r, rgb.g, rgb.b)
            elif array.type == AI_TYPE_RGBA:
               rgba = AiArrayGetRGBA(array, 0)
               return "%g, %g, %g" % (rgba.r, rgba.g, rgba.b, rgba.a)
            elif array.type == AI_TYPE_POINTER:
               ptr = cast(AiArrayGetPtr(array, 0), POINTER(AtNode))
               return "%p" % ptr
            elif array.type == AI_TYPE_NODE:
               ptr = cast(AiArrayGetPtr(array, 0), POINTER(AtNode))
               name = AiNodeGetName(ptr)
               return str(name)
            else:
               return ""
         else:
            return "(%u elements)" % nelems
      else:
         return ""




   def ToggleExpanded(self):
      if self.expanded:
         self.setFixedSize(self.normalSize)
         self.expanded = False
      else:
         self.setFixedSize(self.expandedSize)
         self.expanded = True
      self.updateGeometry()
      self.parent().updateGeometry()

#    def mouseDoubleClickEvent(self, event):
#       self.ToggleExpanded()
