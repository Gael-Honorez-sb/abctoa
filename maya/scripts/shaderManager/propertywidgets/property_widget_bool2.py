from PyQt4.QtGui import *
from PyQt4.QtCore import *
from arnold import *
from property_widget import PropertyWidget


class PropertyWidgetBool2(PropertyWidget):
  def __init__(self, controller, default, name, parent = None):
    PropertyWidget.__init__(self, name, parent)
      
    self.paramName = str(name)
     
    self.controller = controller
    self.controller.setPropertyValue.connect(self.changed)
    self.controller.reset.connect(self.resetValue)

    self.widget = QCheckBox(self)
    self.widget.setTristate(False)

    self.default = default

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