/*Abc Shader Exporter
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#ifndef _abcMayaShaderNode
#define _abcMayaShaderNode


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
