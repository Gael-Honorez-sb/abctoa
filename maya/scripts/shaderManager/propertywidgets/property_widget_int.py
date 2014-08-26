from PyQt4.QtGui import *
from PyQt4.QtCore import *
from arnold import *
from property_widget import *

class PropertyWidgetInt(PropertyWidget):

   def __init__(self, controller,  pentry, param, parent = None):
      PropertyWidget.__init__(self, param, parent)


      self.paramName = param

      self.controller = controller
      self.controller.setPropertyValue.connect(self.changed)
      self.controller.reset.connect(self.resetValue)

      param_value = AiParamGetDefault(pentry)
      param_type = AiParamGetType(pentry)
      self.default = self.GetParamValueAsString(pentry, param_value, param_type)


      self.widget = QSpinBox()
      self.widget.setMaximum(65535)
      self.widget.setValue(self.default)
      self.widget.valueChanged.connect(self.ValueChanged)

      self.layout().addWidget(self.widget)

   def ValueChanged(self, value):
      self.controller.mainEditor.propertyChanged(dict(propname=self.paramName, default=value == self.default, value=value))


   def changed(self, message):

      value = message["value"]
      self.widget.setValue(value)


   def resetValue(self):
    self.widget.setValue(self.default)



