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


#include "abcMayaShaderNode.h"

#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>
#include <maya/MDGModifier.h>

#include <maya/MFnStringData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MString.h>
#include <maya/MColor.h>
#include <maya/MArrayDataBuilder.h>
//Alembic
#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreFactory/IFactory.h>
#include <Alembic/AbcMaterial/IMaterial.h>

//Arnold - For shader parsing

#include <ai.h>

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;



// You MUST change this to a unique value!!!  The id is a 32bit value used
// to identify this type of node in the binary file format.
//

MTypeId     abcMayaShader::id( 0x70811 );



//these are the attributes shared by all instances
MObject    abcMayaShader::sABCFile;
MObject abcMayaShader::aShaderFrom;
MObject abcMayaShader::aShaders;

MObject abcMayaShader::aOutColor;

//this is the default shader, used to render a checkered phong
//shader *abcMayaShader::sDefShader = NULL;

//  This is a list of nodes presently created of this type.
// It allows us to find all shader nodes and update them after a file load,
// and it eases searching for use by the command.
std::vector<abcMayaShader*> abcMayaShader::sNodeList;


abcMayaShader::abcMayaShader(): m_shaderFrom(""), m_class("abcMayaShader") {}
abcMayaShader::~abcMayaShader() {}

MStatus abcMayaShader::compute( const MPlug& plug, MDataBlock& data )
{
  return MS::kUnknownParameter;
}

void* abcMayaShader::creator()
//
//    Description:
//        this method exists to give Maya a way to create new objects
//      of this type.
//
//    Return Value:
//        a new object of this type
//
{
    abcMayaShader *ret = new abcMayaShader;
    sNodeList.push_back(ret);
    return ret;
}

MStatus abcMayaShader::initialize()
//
//    Description:
//        This method is called to create and initialize all of the attributes
//      and attribute dependencies for this node type.  This is only called
//        once when the node type is registered with Maya.
//
//    Return Values:
//        MS::kSuccess
//        MS::kFailure
//
{
    //add the necessary nodes
    MFnTypedAttribute typedAttr;
    MFnNumericAttribute numAttr;
    MFnStringData stringData;
    MStatus stat;
    MObject string;


    // The shader attribute holds the file name of an fx file
    string = stringData.create(&stat);
    if (stat != MS::kSuccess)
    return MS::kFailure;

    sABCFile = typedAttr.create("shader", "s", MFnData::kString, string, &stat);
    if (stat != MS::kSuccess)
    return MS::kFailure;

    stat = typedAttr.setInternal(true);
    if (stat != MS::kSuccess)
    return MS::kFailure;

    // Add the attribute to the node
    if ( addAttribute(sABCFile) != MS::kSuccess)
        return MS::kFailure;

    // Create a shader path attrib to allow access by the AE for UI purposes
    aShaderFrom = typedAttr.create("shaderFrom", "shdfrm", MFnData::kString);
    typedAttr.setInternal(true);

    // shader list
    aShaders = typedAttr.create("shaders", "shaders", MFnStringData::kString);
    typedAttr.setArray(true);
    typedAttr.setInternal(true);
    typedAttr.setUsesArrayDataBuilder( true );

    addAttribute(aShaderFrom);
    addAttribute(aShaders);

    // Output attributes
    aOutColor = numAttr.createColor("outColor", "oc");
    numAttr.setKeyable(false) ;
    numAttr.setStorable(false);
    numAttr.setReadable(true);
    numAttr.setWritable(false);

    addAttribute(aOutColor);

  return MS::kSuccess;
}

//
// callback to configure shaders, post file load
//
////////////////////////////////////////////////////////////////////////////////
void abcMayaShader::rejigShaders( void *data) {

  //iterate over all shaders, and rebuild them, so they can reconnect attributes
  for (std::vector<abcMayaShader*>::iterator it = sNodeList.begin(); it < sNodeList.end(); it++) {
    (*it)->rebuildShader();
  }
}

bool abcMayaShader::rebuildShadersList()
{

    Alembic::Abc::IArchive archive;
    Alembic::AbcCoreFactory::IFactory factory;
    factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
    archive = factory.getArchive(m_abcFile.asChar());
    if (!archive.valid())
    {
        MString theError = "Cannot read file " + m_abcFile;
        MGlobal::displayError(theError);
        return false;
    }

    Abc::IObject materialsObject(archive.getTop(), "materials");
    if(!materialsObject.valid())
    {
        MString theError = "No material library found";
        MGlobal::displayError(theError);
        return false;
    }

    MPlug zPlug (thisMObject(), aShaders);

    unsigned int numShaders = materialsObject.getNumChildren();

    for (unsigned int i = 0; i < numShaders; i++)
    {
        MString shaderName(materialsObject.getChildHeader(i).getName().c_str());
        zPlug.selectAncestorLogicalIndex(i,aShaders);
        zPlug.setValue(shaderName);
    }

    MDataBlock  data=forceCache();
    MArrayDataHandle hVDBAttrs = data.inputArrayValue(aShaders);
    MArrayDataBuilder bVDBAttrs = hVDBAttrs.builder();

    if (bVDBAttrs.elementCount() > numShaders)
    {
        unsigned int current = bVDBAttrs.elementCount();
        for (unsigned int x = numShaders; x < current; x++)
        {
            bVDBAttrs.removeElement(x);
        }

        data.setClean(aShaders);
    }
    return true;
}

bool abcMayaShader::rebuildShader()
{
  //handle shader initialization
    bool rebuiltAnything = false;

    // Re (build) current shader
    if (m_shaderDirty)
    {
        //MGlobal::displayInfo(MString("Rebuild shader") );

        Alembic::Abc::IArchive archive;
        Alembic::AbcCoreFactory::IFactory factory;
        factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
        archive = factory.getArchive(m_abcFile.asChar());
        if (!archive.valid())
        {
            MString theError = "Cannot read file " + m_abcFile;
            MGlobal::displayError(theError);
            return false;
        }
        Abc::IObject materialsObject(archive.getTop(), "materials");
        if(!materialsObject.valid())
        {
            MString theError = "No material library found";
            MGlobal::displayError(theError);
            return false;
        }

        if(m_shaderFrom.empty())
        {
            MString theError = "no material name given";
            MGlobal::displayError(theError);
            return false;
        }

        Abc::IObject object = materialsObject.getChild(m_shaderFrom);

        if (Mat::IMaterial::matches(object.getHeader()))
        {
            Mat::IMaterial matObj(object, Abc::kWrapExisting);

            std::vector<std::string> mappingNames;
            matObj.getSchema().getNetworkInterfaceParameterMappingNames(mappingNames);

            MDGModifier mod;
            MFnDependencyNode dn(thisMObject());

            MFnNumericAttribute nAttrib;
            MFnTypedAttribute tAttrib;

            // Managing the attributes here


            // first we need to set mark all previous attributes as "old"
            for (std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin(); it<m_attributeAttribList.end(); it++)
            {
                //cout << "attribute " << it->name << " marked as inused" << endl;
                it->found = false; //mark it as unused
            }

            for (std::vector<std::string>::iterator I = mappingNames.begin(); I != mappingNames.end(); ++I)
            {
                std::string mapToNodeName;
                std::string mapToParamName;

                if (matObj.getSchema().getNetworkInterfaceParameterMapping((*I), mapToNodeName, mapToParamName))
                {
                    Alembic::AbcMaterial::IMaterialSchema::NetworkNode node = matObj.getSchema().getNetworkNode(mapToNodeName);

                    if (!node.valid())
                        continue;

                    Alembic::Abc::ICompoundProperty props = node.getParameters();
                    if (!props.valid())
                        continue;



                    if (props.getNumProperties() > 0)
                    {
                        MString name((*I).c_str());
                        //cout << "doing " << name << " property is " << mapToParamName << endl;
                        for (int propNum = 0; propNum < props.getNumProperties(); propNum++)
                        {
                            Alembic::AbcCoreAbstract::PropertyHeader header = props.getPropertyHeader(propNum);
                        }


                        Alembic::AbcCoreAbstract::PropertyHeader header = *props.getPropertyHeader(mapToParamName);
                        if (Abc::IFloatProperty::matches(header))
                        {
                            // float type
                            bool exists = checkNumericAttributeExists(name, MFnNumericData::kFloat);

                            if(!exists)
                            {
                                attributeAttrib aa;
                                aa.found = true;
                                aa.name = name;
                                aa.setName = name;
                                if(!dn.hasAttribute( name ))
                                {
                                    aa.attrib = nAttrib.create(name, name, MFnNumericData::kFloat);
                                    nAttrib.setKeyable(true);
                                    nAttrib.addToCategory(mapToNodeName.c_str());
                                    mod.addAttribute(thisMObject(), aa.attrib);

                                    MPlug pl = dn.findPlug(aa.attrib);
                                    Abc::IFloatProperty prop(props, header.getName());

                                    if (prop.valid())
                                    {
                                        const Alembic::AbcCoreAbstract::MetaData md = prop.getMetaData();
                                        std::string val;
                                        val =  md.get("min");
                                        if(val != "")
                                            nAttrib.setMin(atof(val.c_str()));
                                        val =  md.get("max");
                                        if(val != "")
                                            nAttrib.setMax(atof(val.c_str()));
                                        val =  md.get("softmin");
                                        if(val != "")
                                            nAttrib.setSoftMin(atof(val.c_str()));
                                        val =  md.get("softmax");
                                        if(val != "")
                                            nAttrib.setSoftMax(atof(val.c_str()));

                                        nAttrib.setDefault(prop.getValue());
                                    }
                                }
                                else
                                {
                                    aa.attrib = dn.attribute(name);
                                    MFnAttribute fnAttr(aa.attrib);
                                    fnAttr.addToCategory(mapToNodeName.c_str());
                                }

                                m_attributeAttribList.push_back(aa);
                            }

                        }
                        else if (Abc::IStringProperty::matches(header))
                        {
                            bool exists = checkStringAttributeExists(name);

                            if(!exists)
                            {
                                attributeAttrib aa;
                                aa.name = name;
                                aa.setName = name;
                                aa.found = true;
                                if(!dn.hasAttribute( name ))
                                {
                                    // string type
                                    MFnStringData fnStringData;


                                    aa.attrib = tAttrib.create(name, name, MFnStringData::kString);
                                    tAttrib.setKeyable(true);
                                    tAttrib.addToCategory(mapToNodeName.c_str());
                                    mod.addAttribute(thisMObject(), aa.attrib);

                                    MPlug pl = dn.findPlug(aa.attrib);
                                    Abc::IStringProperty prop(props, header.getName());
                                    if (prop.valid())
                                        tAttrib.setDefault(fnStringData.create(prop.getValue().c_str()));
                                }
                                else
                                {
                                    aa.attrib = dn.attribute(name);
                                    MFnAttribute fnAttr(aa.attrib);
                                    fnAttr.addToCategory(mapToNodeName.c_str());
                                }

                                m_attributeAttribList.push_back(aa);
                            }

                        }
                        else if (Abc::IInt32Property::matches(header))
                        {
                            // int type
                            bool exists = checkNumericAttributeExists(name, MFnNumericData::kInt);
                            if(!exists)
                            {
                                attributeAttrib aa;
                                aa.name = name;
                                aa.setName = name;
                                aa.found = true;
                                if(!dn.hasAttribute( name ))
                                {
                                aa.attrib = nAttrib.create(name, name, MFnNumericData::kInt);
                                nAttrib.setKeyable(true);
                                nAttrib.addToCategory(mapToNodeName.c_str());
                                //nAttrib.setInternal(true);
                                mod.addAttribute(thisMObject(), aa.attrib);

                                Abc::IInt32Property prop(props, header.getName());
                                if (prop.valid())
                                    nAttrib.setDefault(prop.getValue());
                                }
                                else
                                {
                                    aa.attrib = dn.attribute(name);
                                    MFnAttribute fnAttr(aa.attrib);
                                    fnAttr.addToCategory(mapToNodeName.c_str());
                                }

                                m_attributeAttribList.push_back(aa);
                            }
                        }
                        else if (Abc::IBoolProperty::matches(header))
                        {
                            // bool type
                            bool exists = checkNumericAttributeExists(name, MFnNumericData::kBoolean);
                            if(!exists)
                            {
                                attributeAttrib aa;
                                aa.name = name;
                                aa.setName = name;
                                aa.found = true;
                                if(!dn.hasAttribute( name ))
                                {
                                    aa.attrib = nAttrib.create(name, name, MFnNumericData::kBoolean);
                                    nAttrib.setKeyable(true);
                                    nAttrib.addToCategory(mapToNodeName.c_str());
                                    //nAttrib.setInternal(true);
                                    mod.addAttribute(thisMObject(), aa.attrib);

                                    Abc::IBoolProperty prop(props, header.getName());
                                    if (prop.valid())
                                        nAttrib.setDefault(prop.getValue());
                                }
                                else
                                {
                                    aa.attrib = dn.attribute(name);
                                    MFnAttribute fnAttr(aa.attrib);
                                    fnAttr.addToCategory(mapToNodeName.c_str());
                                }
                                m_attributeAttribList.push_back(aa);
                            }

                        }
                        else if (Abc::IV3fProperty::matches(header) || Abc::IP3fProperty::matches(header))
                        {
                            //Vector type
                            bool exists = checkNumericAttributeExists(name, MFnNumericData::k3Float);
                            if(!exists)
                            {
                                attributeAttrib aa;
                                aa.name = name;
                                aa.setName = name;
                                aa.found = true;
                                if(!dn.hasAttribute( name ))
                                {
                                    aa.attrib = nAttrib.createPoint(name, name);
                                    nAttrib.setKeyable(true);
                                    nAttrib.addToCategory(mapToNodeName.c_str());
                                    //nAttib.setInternal(true);
                                    mod.addAttribute(thisMObject(), aa.attrib);
                                    MPlug pl = dn.findPlug(aa.attrib);
                                    Abc::IV3fProperty prop(props, header.getName());
                                    if (prop.valid() && !pl.isNull())
                                        nAttrib.setDefault(prop.getValue().x , prop.getValue().y, prop.getValue().z);
                                }
                                else
                                {
                                    aa.attrib = dn.attribute(name);
                                    MFnAttribute fnAttr(aa.attrib);
                                    fnAttr.addToCategory(mapToNodeName.c_str());
                                }
                                m_attributeAttribList.push_back(aa);
                            }

                        }
                        else if (Abc::IC3fProperty::matches(header))
                        {
                            //color type
                            bool exists = checkNumericAttributeExists(name, MFnNumericData::k3Float);
                            if(!exists)
                            {
                                attributeAttrib aa;
                                aa.name = name;
                                aa.setName = name;
                                aa.found = true;
                                if(!dn.hasAttribute( name ))
                                {
                                    aa.attrib = nAttrib.createColor(name, name);
                                    nAttrib.setKeyable(true);
                                    nAttrib.addToCategory(mapToNodeName.c_str());
                                    mod.addAttribute(thisMObject(), aa.attrib);

                                    MPlug pl = dn.findPlug(aa.attrib);
                                    Abc::IC3fProperty prop(props, header.getName());
                                    if (prop.valid() && !pl.isNull())
                                        nAttrib.setDefault(prop.getValue().x , prop.getValue().y, prop.getValue().z);
                                }
                                else
                                {
                                    aa.attrib = dn.attribute(name);
                                    MFnAttribute fnAttr(aa.attrib);
                                    fnAttr.addToCategory(mapToNodeName.c_str());
                                }
                                m_attributeAttribList.push_back(aa);
                            }
                        }
                    }
                }
            }
            // Now we clear all the plugs that has not been found.
            for (unsigned int ii=0; ii < m_attributeAttribList.size();)
            {
                if (m_attributeAttribList[ii].found == false)
                {
                    mod.removeAttribute(thisMObject(), m_attributeAttribList[ii].attrib);
                    m_attributeAttribList.erase(m_attributeAttribList.begin()+ii);
                }
                else
                    ii++;

            }

            mod.doIt();

        }
        else
        {
            MString theError = "material not found";
            MGlobal::displayError(theError);
            return false;
        }

    }
    //MGlobal::displayInfo(MString("done rebuild") );
 return true;
}


bool abcMayaShader::checkStringAttributeExists(MString name)
{
    std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin();
    while (it < m_attributeAttribList.end())
    {
        if (it->name == name)
            break;
        it++;
    }

    if (it !=m_attributeAttribList.end())
    {
        MDGModifier mod;
        MFnDependencyNode dn(thisMObject());

        //if found, we check if the type is the same.

        if(it->attrib.hasFn(MFn::kStringData))
        {
            MFnNumericAttribute fnAttrib(it->attrib);
            if(fnAttrib.type() == MFnStringData::kString)
            {
                it->found = true;
                return true;
            }
        }

        // we've found something, but not the same time -> delete this fucker.
        if ( dn.hasAttribute( name ))
        {
            mod.removeAttribute(thisMObject(), it->attrib);
            mod.doIt();
            m_attributeAttribList.erase(it);
        }

    }
    return false;


}

bool abcMayaShader::checkNumericAttributeExists(MString name, MFnNumericData::Type type)
{
    std::vector<attributeAttrib>::iterator it=m_attributeAttribList.begin();
    while (it < m_attributeAttribList.end())
    {
        if (it->name == name)
            break;
        it++;
    }

    if (it !=m_attributeAttribList.end())
    {
        MDGModifier mod;
        MFnDependencyNode dn(thisMObject());

        //if found, we check if the type is the same.
        if(it->attrib.hasFn(MFn::kNumericAttribute))
        {

            MFnNumericAttribute fnAttrib(it->attrib);
            if(fnAttrib.unitType() == type)
            {
                //cout << "attribute " << name << " exists" << endl;
                it->found = true;
                return true;
            }

        }
        // we've found something, but not the same type -> delete this fucker.
        if ( dn.hasAttribute( name ))
        {
            //cout << "attribute " << name << " deletion" << endl;
            mod.removeAttribute(thisMObject(), it->attrib);
            mod.doIt();

        }
        m_attributeAttribList.erase(it);
    }
    return false;

}

bool abcMayaShader::setInternalValueInContext( const MPlug &plug,
                                               const MDataHandle &handle,
                                               MDGContext & )
{
  bool retVal = false;

  if ( plug == aShaderFrom)
  {
    m_shaderFrom = handle.asString().asChar();
    m_shaderDirty = true;
    rebuildShader();
    retVal = true;
  }
  else if (plug == sABCFile)
  {
      m_abcFile = handle.asString();
      m_shaderDirty = true;
      rebuildShadersList();
      retVal = true;
  }

  return retVal;
}

////////////////////////////////////////////////////////////////////////////////
/* virtual */
bool abcMayaShader::getInternalValueInContext( const MPlug &plug,
                                               MDataHandle &handle,
                                               MDGContext& )
{

    bool retVal = false;
    if ( plug == aShaderFrom )
    {
        handle.set(MString(m_shaderFrom.c_str()));
        retVal = true;
    }


    if ( plug == sABCFile )
    {
        handle.set(m_abcFile);
        retVal = true;
    }

    return retVal;
}


abcMayaShader* abcMayaShader::findNodeByName(const MString &name)
{

  for (std::vector<abcMayaShader*>::iterator it = sNodeList.begin(); it < sNodeList.end(); it++)
  {
    if ( (*it)->name() == name)
      return *it;
  }

  return NULL;
}

