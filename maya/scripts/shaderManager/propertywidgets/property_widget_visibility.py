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


class PropertyWidgetVisibility(PropertyWidget):
  def __init__(self, controller, params, parent = None):
    PropertyWidget.__init__(self, params, parent)

    self.paramName = params["name"]

    self.minVal = AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT|AI_RAY_VOLUME|AI_RAY_SPECULAR_TRANSMIT|AI_RAY_DIFFUSE_TRANSMIT|AI_RAY_SHADOW|AI_RAY_CAMERA)

    self.controller = controller
    self.controller.setPropertyValue.connect(self.changed)
    self.controller.reset.connect(self.resetValue)

    self.allSwitch = QPushButton(self)
    self.allSwitch.setText("All Off")
    self.allSwitch.pressed.connect(self.switchPressed)

    self.switch = True

    self.viz = {}
    self.viz["camera"] = QCheckBox(self)
    self.viz["cast_shadows"] = QCheckBox(self)
    self.viz["diffuse_transmit"] = QCheckBox(self)
    self.viz["specular_transmit"] = QCheckBox(self)
    self.viz["volume"] = QCheckBox(self)
    self.viz["diffuse_reflect"] = QCheckBox(self)
    self.viz["specular_reflect"] = QCheckBox(self)
    self.viz["subsurface"] = QCheckBox(self)

    self.default = params["value"]
    self.setViz(self.default)

    grid= QGridLayout()
    grid.addWidget(self.allSwitch, 1, 1)
    row = 2
    for v in self.viz:
      label = QLabel(v)
      grid.addWidget(label, row, 1)
      grid.addWidget(self.viz[v], row, 2)
      self.viz[v].stateChanged.connect(self.PropertyChanged)
      row = row + 1


    self.layout().addLayout(grid)

  def switchPressed(self):
    if self.switch:
        self.setViz(self.minVal)
        self.allSwitch.setText("All On")
        self.switch = False
    else:
        self.setViz(255)
        self.allSwitch.setText("All Off")
        self.switch = True

  def computeViz(self):
    value = AI_RAY_ALL

    if not self.viz["camera"].isChecked():
        value &= ~AI_RAY_CAMERA
    if not self.viz["cast_shadows"].isChecked():
        value &= ~AI_RAY_SHADOW
    if not self.viz["diffuse_transmit"].isChecked():
        value &= ~AI_RAY_DIFFUSE_TRANSMIT
    if not self.viz["specular_transmit"].isChecked():
        value &= ~AI_RAY_SPECULAR_TRANSMIT
    if not self.viz["volume"].isChecked():
        value &= ~AI_RAY_VOLUME
    if not self.viz["diffuse_reflect"].isChecked():
        value &= ~AI_RAY_DIFFUSE_REFLECT
    if not self.viz["specular_reflect"].isChecked():
        value &= ~AI_RAY_SPECULAR_REFLECT
    if not self.viz["subsurface"].isChecked():
        value &= ~AI_RAY_SUBSURFACE

    return value

  def setViz(self, value):

    if value <= self.minVal:
        self.allSwitch.setText("All On")
        self.switch = False
    else:
        self.allSwitch.setText("All Off")
        self.switch = True

    for v in self.viz:
      self.viz[v].setChecked(0)

    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE):
        self.viz["subsurface"].setChecked(1)
        value = value - AI_RAY_SUBSURFACE
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT):
        self.viz["specular_reflect"].setChecked(1)
        value = value - AI_RAY_SPECULAR_REFLECT
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT):
        self.viz["diffuse_reflect"].setChecked(1)
        value = value - AI_RAY_DIFFUSE_REFLECT
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT|AI_RAY_VOLUME):
        self.viz["volume"].setChecked(1)
        value = value - AI_RAY_VOLUME        
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT|AI_RAY_VOLUME|AI_RAY_SPECULAR_TRANSMIT):
        self.viz["specular_transmit"].setChecked(1)
        value = value - AI_RAY_SPECULAR_TRANSMIT
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT|AI_RAY_VOLUME|AI_RAY_SPECULAR_TRANSMIT|AI_RAY_DIFFUSE_TRANSMIT):
        self.viz["diffuse_transmit"].setChecked(1)
        value = value - AI_RAY_DIFFUSE_TRANSMIT
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT|AI_RAY_VOLUME|AI_RAY_SPECULAR_TRANSMIT|AI_RAY_DIFFUSE_TRANSMIT|AI_RAY_SHADOW):
        self.viz["cast_shadows"].setChecked(1)
        value = value - AI_RAY_SHADOW
    if value > AI_RAY_ALL & ~(AI_RAY_SUBSURFACE|AI_RAY_SPECULAR_REFLECT|AI_RAY_DIFFUSE_REFLECT|AI_RAY_VOLUME|AI_RAY_SPECULAR_TRANSMIT|AI_RAY_DIFFUSE_TRANSMIT|AI_RAY_SHADOW|AI_RAY_CAMERA):
        self.viz["camera"].setChecked(1)
        value = value - AI_RAY_CAMERA





  def PropertyChanged(self, state):
    value= self.computeViz()

    if value <= self.minVal:
        self.allSwitch.setText("All On")
        self.switch = False
    else:
        self.allSwitch.setText("All Off")
        self.switch = True
        
    self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


  def changed(self, message):

    value = message["value"]

    for v in self.viz:
      self.viz[v].stateChanged.disconnect()
    self.setViz(value)
    for v in self.viz:
      self.viz[v].stateChanged.connect(self.PropertyChanged)


  def resetValue(self):
    for v in self.viz:
      self.viz[v].stateChanged.disconnect()
    self.setViz(self.default)
    for v in self.viz:
      self.viz[v].stateChanged.connect(self.PropertyChanged)
