#include <ai.h>
#include <stdio.h>

extern AtNodeMethods* ABCShaderMethods;
//extern AtNodeMethods* ShaderAssignMethods;


enum SHADERS
{
   ABCSHADER
   //ASSIGNSHADER
};

node_loader
{
   sprintf(node->version, AI_VERSION);

   switch (i)
   {
   case ABCSHADER :
      node->methods     = (AtNodeMethods*) ABCShaderMethods;
      node->output_type = AI_TYPE_RGB;
      node->name        = "AbcShader";
      node->node_type   = AI_NODE_SHADER;
      break;/*
   case ASSIGNSHADER :
      node->methods     = (AtNodeMethods*) ShaderAssignMethods;
      node->output_type = AI_TYPE_RGB;
      node->name        = "AssignShader";
      node->node_type   = AI_NODE_SHADER;
      break;   */
   default:
      return false;
   }

   return true;
}
