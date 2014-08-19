#ifndef _abcMayaShaderNode
#define _abcMayaShaderNode
//
// Copyright (C) nozon
// 
// File: abcMayaShaderNode.h
//
// Dependency Graph Node: abcMayaShader
//
// Author: Maya Plug-in Wizard 2.0
//

#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MTypeId.h> 
#include <maya/MNodeClass.h> 
#include <vector>

 
class abcMayaShader : public MPxNode
{
public:
                        abcMayaShader();
    virtual                ~abcMayaShader(); 

    virtual MStatus        compute( const MPlug& plug, MDataBlock& data );

    static  void*        creator();
    static  MStatus        initialize();

    static abcMayaShader* findNodeByName(const MString &name);

    virtual bool getInternalValueInContext( const MPlug&,
                                              MDataHandle&,
                                              MDGContext&);
    virtual bool setInternalValueInContext( const MPlug&,
                                              const MDataHandle&,
                                              MDGContext&);

public:

    // There needs to be a MObject handle declared for each attribute that
    // the node will have.  These handles are needed for getting and setting
    // the values later.
    //
    //attributes for selecting shaders
    static  MObject    sABCFile;
    static  MObject aShaderFrom;
    static  MObject aShaders;

    static  MObject aOutColor;

   // Input attributes
   static MObject SAttr[];

    bool m_shaderDirty;
    //call back to configure shaders, post load
    static void rejigShaders( void *data);

    MString m_abcFile;
    std::string m_shaderFrom;

    struct attributeAttrib 
    {
      MString name;
      MString setName;
      MObject attrib;
      bool found;
    };

    std::vector<attributeAttrib> m_attributeAttribList;

    // The typeid is a unique 32bit indentifier that describes this node.
    // It is used to save and retrieve nodes of this type from the binary
    // file format.  If it is not unique, it will cause file IO problems.
    //
    static    MTypeId        id;

    //this list allows us to access all nodes on callbacks
    // would it be better to just query from Maya?
    static std::vector<abcMayaShader*> sNodeList;
    bool rebuildShader();
    bool rebuildShadersList();

    bool checkNumericAttributeExists(MString name, MFnNumericData::Type type);
    bool checkStringAttributeExists(MString name);

    MNodeClass m_class;
};

#endif
