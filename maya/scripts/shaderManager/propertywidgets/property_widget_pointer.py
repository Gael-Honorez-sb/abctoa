
from property_widget import *

class PropertyWidgetPointer(PropertyWidget):
   def __init__(self, node, name, parent = None):
      PropertyWidget.__init__(self, name, parent)

      self.node = node
      self.paramName = name
         
      self.widget = QComboBox()

      self.widget.addItem("[None]")
      iter = AiUniverseGetNodeIterator(AI_NODE_ALL)
      while not AiNodeIteratorFinished(iter):
         node = AiNodeIteratorGetNext(iter)
         if node: 
            self.widget.addItem(AiNodeGetName(node))
         
      AiNodeIteratorDestroy(iter)

      self.__ReadFromArnold()
      
      self.widget.currentIndexChanged.connect(self.ValueChanged)
      self.layout().addWidget(self.widget)

   def ValueChanged(self, value):
      self.__WriteToArnold()
      self.propertyChanged.emit()

   def __ReadFromArnold(self):
      nodePtr = AiNodeGetPtr(self.node, self.paramName)
      if nodePtr:
         nodePtr = cast(nodePtr, POINTER(AtNode))
      
         nodeName = AiNodeLookUpByPointer(nodePtr)
         if nodeName:
            n = self.widget.findText(nodeName, Qt.MatchExactly)
            if n != -1:
               self.widget.setCurrentIndex(n)
      else:
         self.widget.setCurrentIndex(0)  # Set to [None]
   
   def __WriteToArnold(self):
      if self.widget.currentIndex() == 0:
         AiNodeSetPtr(self.node, self.paramName, None)
      else:
         nodePtr = AiNodeLookUpByName(str(self.widget.currentText()))
         AiNodeSetPtr(self.node, self.paramName, nodePtr)
