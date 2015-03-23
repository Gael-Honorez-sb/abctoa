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

AI_TYPE_BYTE =          0x00  ## Byte (an 8-bit sized unsigned integer)
AI_TYPE_INT =           0x01  ## Integer
AI_TYPE_UINT =          0x02  ## Unsigned integer
AI_TYPE_BOOLEAN =       0x03  ## Boolean (either TRUE or FALSE)
AI_TYPE_FLOAT =         0x04  ## Single-precision floating point number
AI_TYPE_RGB =           0x05  ## RGB struct
AI_TYPE_RGBA =          0x06  ## RGBA struct
AI_TYPE_VECTOR =        0x07  ## XYZ vector
AI_TYPE_POINT =         0x08  ## XYZ point
AI_TYPE_POINT2 =        0x09  ## XY point
AI_TYPE_STRING =        0x0A  ## C-style character string
AI_TYPE_POINTER =       0x0B  ## Arbitrary pointer
AI_TYPE_NODE =          0x0C  ## Pointer to an Arnold node
AI_TYPE_ARRAY =         0x0D  ## AtArray
AI_TYPE_MATRIX =        0x0E  ## 4x4 matrix
AI_TYPE_ENUM =          0x0F  ## Enumeration (see \ref AtEnum)
AI_TYPE_UNDEFINED =     0xFF  ## Undefined, you should never encounter a parameter of this type
AI_TYPE_NONE =          0xFF  ## No type

from PySide.QtGui import *
from PySide.QtCore import *
import json, os

from shaderManager.propertywidgets import property_widget
from shaderManager.propertywidgets import property_widget_bool
from shaderManager.propertywidgets import property_widget_color 
from shaderManager.propertywidgets import property_widget_enum 
from shaderManager.propertywidgets import property_widget_float 
from shaderManager.propertywidgets import property_widget_int 
from shaderManager.propertywidgets import property_widget_node 
from shaderManager.propertywidgets import property_widget_pointer 
from shaderManager.propertywidgets import property_widget_string 
from shaderManager.propertywidgets import property_widget_vector 
from shaderManager.propertywidgets import property_widget_visibility 

reload(property_widget)
reload(property_widget_bool)
reload(property_widget_color)
reload(property_widget_enum)
reload(property_widget_float)
reload(property_widget_int)
reload(property_widget_node)
reload(property_widget_pointer)
reload(property_widget_string)
reload(property_widget_vector)
reload(property_widget_visibility)

PROPERTY_ADD_LIST = {
'polymesh'       : [
                    {'name' :'forceVisible', 'type': AI_TYPE_BOOLEAN, 'value' : False},
                    {'name' :'sss_setname', 'type': AI_TYPE_STRING, 'value' : ""},
                  ],
'points'         : [
                    {'name' :'forceVisible', 'type': AI_TYPE_BOOLEAN, 'value' : False}, 
                    {'name' :'radius_multiplier', 'type': AI_TYPE_FLOAT, 'value' : 1.0},
                    {'name' :'velocity_multiplier', 'type': AI_TYPE_FLOAT, 'value' : 1.0}
                    ],
'mesh_light'    :  [
                    {'name' :'convert_to_mesh_light', 'type': AI_TYPE_BOOLEAN, 'value' : False}
                   ]                    
}

PROPERTY_BLACK_LIST = {
'options'        : ['outputs'],
'polymesh'       : ['nidxs', 'nlist', 'nsides', 'shidxs', 'uvidxs', 'uvlist', 'vidxs', 'crease_idxs', 'vlist', 'autobump_visibility', 'sidedness', 'ray_bias'],
'points'         : ['sidedness', 'ray_bias'],
'driver_display' : ['callback', 'callback_data'],
'mesh_light'    :  []
}

class PropertyEditor(QWidget):
    propertyChanged = Signal(dict)
    setPropertyValue = Signal(dict)
    reset = Signal()
    def __init__(self, mainEditor, nodetype, parent = None):
        QWidget.__init__(self, parent)
        self.mainLayout = QVBoxLayout()
        self.setLayout(self.mainLayout)

        
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)),"arnold_nodes.json"), "r") as node_definition:
          self.nodes = json.load(node_definition)

        self.mainEditor = mainEditor
        
        self.name = nodetype
        self.node = self.nodes[self.name]

        self.scrollArea = None
        labelLayout = QHBoxLayout()
        self.mainLayout.addLayout(labelLayout)
        self.switch = QPushButton(self)
        self.switch.pressed.connect(self.switchPressed)
        labelLayout.addWidget(QLabel("Node: %s" % self.name))
        labelLayout.addWidget(self.switch)
        labelLayout.addStretch()

        self.propertyWidgets = {}
        self.resetTo(nodetype)

    def propertyValue(self, message):
      param = message["paramname"]
      if param in self.propertyWidgets:
        self.propertyWidgets[param].changed(message)
        self.propertyWidgets[param].title.setText("<font color='red'>%s</font>" % message["paramname"])

    def resetToDefault(self):
      self.reset.emit()
      for param in self.propertyWidgets:
        if hasattr(self.propertyWidgets[param], "title"):
          self.propertyWidgets[param].title.setText(param)

    def clearAll(self):
      for name in self.propertyWidgets:
        self.propertyWidgets[name].deleteLater()
      self.propertyWidgets = {}

      if self.scrollArea :
        self.scrollArea.deleteLater()

    def resetTo(self, nodetype):
      self.resetToDefault()
      self.clearAll()
      self.name = nodetype
      self.node = self.nodes[self.name]

      if self.node:
          if nodetype == "polymesh":
            #for polymesh, we are adding meshlight.
            self.switch.setText("Mesh Light")
            self.switch.setVisible(1)
          elif nodetype == "mesh_light":
            self.switch.setText("Polymesh")
            self.switch.setVisible(1)
          else:
            self.switch.setVisible(0)

          frameLayout = QVBoxLayout()
          self.scrollArea = QScrollArea()
          self.scrollArea.setWidgetResizable(True)
          frame = QFrame()
          frame.setLayout(frameLayout)
          self.mainLayout.addWidget(self.scrollArea)

          for prop in PROPERTY_ADD_LIST[self.name]:
            propertyWidget = self.GetPropertyWidget(prop, frame, False)

            self.propertyWidgets[prop["name"]] = propertyWidget
            if propertyWidget:
              frameLayout.addWidget(propertyWidget)

        ## Built-in parameters
          for param in self.node["params"]:

              paramName = param["name"]

              blackList = PROPERTY_BLACK_LIST[self.name] if self.name in PROPERTY_BLACK_LIST else []
              if paramName != 'name' and not paramName in blackList:
                propertyWidget = self.GetPropertyWidget(param, frame, False)
                if propertyWidget:
                  self.propertyWidgets[paramName] = propertyWidget
                  frameLayout.addWidget(propertyWidget)


          frameLayout.addStretch(0)

          self.scrollArea.setWidget(frame)
      else:
          self.mainLayout.addStretch()

      self.mainEditor.updateAttributeEditor()

    def GetPropertyWidget(self, param, parent, userData):
      widget = None
      type = param["type"]
      if "visibility" in param["name"] or "sidedness" in param["name"]:
         widget = property_widget_visibility.PropertyWidgetVisibility(self, param, parent)
      elif type == AI_TYPE_BYTE:
         widget = property_widget_int.PropertyWidgetInt(self, param, parent)
      elif type == AI_TYPE_INT:
         widget = property_widget_int.PropertyWidgetInt(self, param, parent)
      elif type == AI_TYPE_UINT:
         widget = property_widget_int.PropertyWidgetInt(self, param,  parent)
      elif type == AI_TYPE_FLOAT:
         widget = property_widget_float.PropertyWidgetFloat(self, param, parent)
      elif type == AI_TYPE_VECTOR:
         widget = property_widget_vector.PropertyWidgetVector(self, param, property_widget.PropertyWidget.VECTOR, parent)
      elif type == AI_TYPE_POINT:
         widget = property_widget_vector.PropertyWidgetVector(self, param, property_widget.PropertyWidget.POINT, parent)
      elif type == AI_TYPE_POINT2:
         widget = property_widget_vector.PropertyWidgetVector(self, param, property_widget.PropertyWidget.POINT2, parent)
      if type == AI_TYPE_BOOLEAN:
         widget = property_widget_bool.PropertyWidgetBool(self, param, parent)
      elif type == AI_TYPE_STRING:
         widget = property_widget_string.PropertyWidgetString(self, param, parent)
      elif type == AI_TYPE_ENUM:
         widget = property_widget_enum.PropertyWidgetEnum(self, param, parent)
      elif type == AI_TYPE_RGB:
         widget = property_widget_color.PropertyWidgetColor(self, param, property_widget.PropertyWidget.RGB, parent)
      elif type == AI_TYPE_RGBA:
         widget = property_widget_color.PropertyWidgetColor(self, param, property_widget.PropertyWidget.RGBA, parent)
      ##elif type == AI_TYPE_POINTER:
      ##   widget = PropertyWidgetPointer(nentry, name, parent)
      # elif type == AI_TYPE_NODE:
      #    widget = PropertyWidgetNode(self, pentry, name, parent)
      if widget and userData:
         widget.setBackgroundRole(QPalette.Base)
      return widget

    def switchPressed(self):
      if self.name == "polymesh":
        self.switch.setText("Polymesh")
        self.resetTo("mesh_light")
      else:
        self.switch.setText("Mesh Light")
        self.resetTo("polymesh")
