from PySide.QtGui import *
from PySide.QtCore import *
from arnold import *
from property_widget import *
class PropertyWidgetColor(PropertyWidget):
   def __init__(self, controller,  pentry, name, colorType, parent = None):
      PropertyWidget.__init__(self, name, parent)

      self.paramName = name
      self.colorType = colorType

      self.button = QPushButton(self)
      self.button.setFlat(True)
      self.button.setAutoFillBackground(True)
      self.button.clicked.connect(self.ShowColorDialog)
      self.layout().addWidget(self.button)

      data = AtColor()
      if AiMetaDataGetRGB(nentry, name, "default", byref(data)):
         data = data.clamp(0, 1)
         color = QColor(data.r * 255, data.g * 255, data.b * 255)
         self.ColorChanged(color)
      else:
         self.__ReadFromArnold()
   def ShowColorDialog(self):
      try:
         Global.propertyEditor.colorDialog.currentColorChanged.disconnect()
      except:
         pass
      Global.propertyEditor.colorDialog.currentColorChanged.connect(self.ColorChanged)
      color = self.button.palette().color(QPalette.Button)
      Global.propertyEditor.colorDialog.setCurrentColor(color)
      Global.propertyEditor.colorDialog.setOption(QColorDialog.ShowAlphaChannel, self.colorType == PropertyWidget.RGBA)
      Global.propertyEditor.colorDialog.show()
   def ColorChanged(self, color):
      palette = QPalette()
      palette.setColor(QPalette.Button, color)
      self.button.setPalette(palette)

      self.__WriteToArnold()
      self.propertyChanged.emit(self.paramName)

   def __ReadFromArnold(self):
      palette = QPalette()
      if self.colorType == PropertyWidget.RGB:
         color = AiNodeGetRGB(self.node, self.paramName).clamp(0, 1)
      elif self.colorType == PropertyWidget.RGBA:
         color = AiNodeGetRGBA(self.node, self.paramName).clamp(0, 1)

      palette.setColor(QPalette.Button, QColor(color.r * 255, color.g * 255, color.b * 255))
      self.button.setPalette(palette)
   def __WriteToArnold(self):
      color = self.button.palette().color(QPalette.Button)
      if self.colorType == PropertyWidget.RGB:
         AiNodeSetRGB(self.node, self.paramName, float(color.red()) / 255, float(color.green()) / 255, float(color.blue()) / 255)
      elif self.colorType == PropertyWidget.RGBA:
         AiNodeSetRGBA(self.node, self.paramName, float(color.red()) / 255, float(color.green()) / 255, float(color.blue()) / 255, float(color.alpha()) / 255)