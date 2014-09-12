#include "parseJsonShaders.h"

#include <maya/MDagPath.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFnDependencyNode.h>


// Hashing
const size_t hash( std::string const& s )
{
    size_t result = 2166136261U ;
    std::string::const_iterator end = s.end() ;
    for ( std::string::const_iterator iter = s.begin() ;
            iter != end ;
            ++ iter )
    {
        result = 127 * result
                + static_cast< unsigned char >( *iter ) ;
    }
    return result ;
 }


MObject findShader( MObject& setNode )
//
//  Description:
//      Find the shading node for the given shading group set node.
//
{
        MFnDependencyNode fnNode(setNode);
        MPlug shaderPlug = fnNode.findPlug("surfaceShader");
                        
        if (!shaderPlug.isNull()) {                     
                MPlugArray connectedPlugs;
                bool asSrc = false;
                bool asDst = true;
                shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );

                if (connectedPlugs.length() != 1)
                        cerr << "Error getting shader\n";
                else 
                        return connectedPlugs[0].node();
        }                       
        
        return MObject::kNullObj;
}


bool findShaderColor(MString shaderName, MColor & shaderColor)
{
    MStatus status;
    MSelectionList slist;
    status = slist.add(shaderName);
    if(status)
    {
        // we've added the shader, so it exists!
        MObject shaderNodeSG, shaderNode;
        slist.getDependNode(0, shaderNodeSG);
            
        if (shaderNodeSG == MObject::kNullObj)
            return false;

        shaderNode = findShader(shaderNodeSG);
        if (shaderNode == MObject::kNullObj)
            return false;
            
        MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", &status);
        if (status == MS::kFailure)
            return false;
            
        MColor shaderColor;
        MPlugArray cc;
        colorPlug.connectedTo( cc, true /* asDst */, false );
        if ( cc.length() > 0 )
            return false;

        colorPlug.child(0).getValue( shaderColor.r );
        colorPlug.child(1).getValue( shaderColor.g );
        colorPlug.child(2).getValue( shaderColor.b );
        return true;
    }

    // no shader found, we return black!
    return false;
}

void ParseShaders(Json::Value jroot, std::map<std::string, MColor> & shaderColors)
{
    for( Json::ValueIterator itr = jroot.begin() ; itr != jroot.end() ; itr++ )
    {
        MColor shaderColor;
        bool foundShaderColor = findShaderColor(MString(itr.key().asCString()), shaderColor);

        if(!foundShaderColor)
        {
            //we general a random color from the shader name.
            srand(hash(itr.key().asCString()));
            shaderColor.r = ((double) rand() / (RAND_MAX));
            shaderColor.g = ((double) rand() / (RAND_MAX));
            shaderColor.b = ((double) rand() / (RAND_MAX));
        }


        const Json::Value paths = jroot[itr.key().asString()];
        for( Json::ValueIterator itr2 = paths.begin() ; itr2 != paths.end() ; itr2++ )
        {
            Json::Value val = paths[itr2.key().asUInt()];
            shaderColors[val.asString()] = shaderColor;
        }

    }
}


