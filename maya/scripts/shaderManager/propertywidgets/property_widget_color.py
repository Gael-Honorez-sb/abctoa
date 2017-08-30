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

from Qt.QtGui import *
from Qt.QtCore import *
from Qt.QtWidgets import *
from arnold import *
from property_widget import *
class PropertyWidgetColor(PropertyWidget):
   def __init__(self, controller,  params, colorType, parent = None):
      PropertyWidget.__init__(self, params, parent)

      self.paramName = params["name"]
      self.colorType = colorType

      self.controller = controller

      self.widget = QPushButton(self)
      self.widget.setFlat(True)
      self.widget.setAutoFillBackground(True)
      self.widget.clicked.connect(self.ShowColorDialog)
      self.layout().addWidget(self.widget)

      self.default = params["value"]

      color = QColor(self.default[0] * 255, self.default[1] * 255, self.default[2]* 255)

      self.colorDialog = QColorDialog(self)
      self.ColorChanged(color)
      ##data = AtColor()
      #if AiMetaDataGetRGB(nentry, name, "default", byref(data)):
         #data = data.clamp(0, 1)
         #color = QColor(data.r * 255, data.g * 255, data.b * 255)
         #self.ColorChanged(color)
      #else:
         #self.__ReadFromArnold()

      #self.widget.stateChanged.connect(self.PropertyChanged)

   def ColorChanged(self, color):
      palette = QPalette()
      palette.setColor(QPalette.Button, color)
      self.widget.setPalette(palette)

   def ShowColorDialog(self):
      try:
         self.colorDialog.ValueChanged.disconnect()
      except:
         pass
      self.colorDialog.currentColorChanged.connect(self.ValueChanged)
      color = self.widget.palette().color(QPalette.Button)
      self.colorDialog.setCurrentColor(color)
      self.colorDialog.setOption(QColorDialog.ShowAlphaChannel, self.colorType == PropertyWidget.RGBA)
      self.colorDialog.show()


   def ValueChanged(self, color):
      self.ColorChanged(color)
      value = []
      value.append( color.redF())
      value.append( color.greenF())
      value.append( color.blueF())
      if  self.colorType == PropertyWidget.RGBA:
         value.append( color.alphaF())
      self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))

   def changed(self, message):
      value = message["value"]
      color = QColor(value[0]*255, value[1]*255, value[2]*255)
      self.ColorChanged(color)


   def resetValue(self):
    self.widget.setValue(self.default)


