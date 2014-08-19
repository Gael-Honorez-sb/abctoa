#ifndef _GlShaderHolder_h_
#define _GlShaderHolder_h_

#include <maya/MHardwareRenderer.h>
#include <maya/MGLFunctionTable.h>
//#include <GL/glx.h>
//#include <GL/glew.h>
#include <maya/MPxLocatorNode.h>
//#include <GL/glut.h>

class GlShaderHolder
{
    public:
        void init(char* vs, char* fs);
        
        MGLuint getProgram() { return m_p; }
        
    
    private:
    
        MGLuint m_v;
        MGLuint m_f;
        MGLuint m_p;

};


#endif
