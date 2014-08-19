from PyQt4.QtGui import *
from PyQt4.QtCore import *
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
