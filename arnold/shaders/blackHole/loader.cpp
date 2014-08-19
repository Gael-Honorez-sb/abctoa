#include <ai.h>
#include <stdio.h>

extern AtNodeMethods* blackholeMtd;

enum SHADERS
{
   BLACKHOLE
};

node_loader
{
   sprintf(node->version, AI_VERSION);

   switch (i)
   {
   case BLACKHOLE :
        // 0x70509
      node->methods     = (AtNodeMethods*) blackholeMtd;
      node->output_type = AI_TYPE_RGBA;
      node->name        = "Blackhole";
      node->node_type   = AI_NODE_SHADER;
      break;
   default:
      return false;
   }

   return true;
}
