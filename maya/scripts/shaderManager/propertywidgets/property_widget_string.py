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

from PySide.QtGui import *
from PySide.QtCore import *
from arnold import *
from property_widget import *
class PropertyWidgetString(PropertyWidget):
   def __init__(self, controller, pentry, name, parent = None):
      PropertyWidget.__init__(self, name, parent)
      self.node = node
      self.paramName = name

      self.widget = QLineEdit(self)
      param_value = AiParamGetDefault(pentry)
      param_type = AiParamGetType(pentry)
      default = self.GetParamValueAsString(pentry, param_value, param_type)



      self.widget.returnPressed.connect(self.TextChanged)
      self.layout().addWidget(self.widget)
   def TextChanged(self):

      self.propertyChanged.emit(self.paramName)
   def __ReadFromArnold(self):
      value = AiNodeGetStr(self.node, self.paramName)
      self.widget.setText(value if value else '')
   def __WriteToArnold(self):
      AiNodeSetStr(self.node, self.paramName, str(self.widget.text()))
