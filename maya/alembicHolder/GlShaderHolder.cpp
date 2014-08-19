#include "GlShaderHolder.h"
#include <iostream>



void GlShaderHolder::init(char* vs, char* fs)
{
   static MGLFunctionTable *gGLFT = NULL;
   if (gGLFT == NULL)
      gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();
    
    std::cout << "pinit0" << std::endl;
    m_v = gGLFT->glCreateShaderObjectARB(MGL_VERTEX_SHADER_ARB);
    m_f = gGLFT->glCreateShaderObjectARB(MGL_FRAGMENT_SHADER_ARB);
    std::cout << "pinit" << std::endl;

    const char * ff = fs;
    const char * vv = vs;

    gGLFT->glShaderSourceARB(m_v, 1, &vv,NULL);
    gGLFT->glShaderSourceARB(m_f, 1, &ff,NULL);
    std::cout << "pinit2" << std::endl;

    gGLFT->glCompileShaderARB(m_v);
    gGLFT->glCompileShaderARB(m_f);
    std::cout << "pinit3" << std::endl;
    
    m_p = gGLFT->glCreateProgramObjectARB();
    gGLFT->glAttachObjectARB(m_p,m_f);
    gGLFT->glAttachObjectARB(m_p,m_v);
    std::cout << "pinit4" << std::endl;

    gGLFT->glLinkProgramARB(m_p);
}
