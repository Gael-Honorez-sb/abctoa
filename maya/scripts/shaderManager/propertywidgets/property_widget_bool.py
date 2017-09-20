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
from arnold import *
from property_widget import PropertyWidget
import property_widget
reload(property_widget)

class PropertyWidgetBool(PropertyWidget):
  def __init__(self, controller, param, parent = None):
    PropertyWidget.__init__(self, param, parent)

    self.paramName = param["name"]
    self.controller = controller
    self.controller.setPropertyValue.connect(self.changed)
    self.controller.reset.connect(self.resetValue)

    self.widget = QCheckBox(self)
    self.widget.setTristate(False)

    self.default = param["value"]

    self.widget.setChecked(self.default)
    #self.PropertyChanged(self.default)

    self.widget.stateChanged.connect(self.PropertyChanged)
    self.layout().addWidget(self.widget)



  def PropertyChanged(self, state):
    if state == 2:
      value = True
    else:
      value = False
    self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


  def changed(self, message):
    value = message["value"]
    self.widget.setChecked(value)


  def resetValue(self):
    self.widget.setChecked(self.default)