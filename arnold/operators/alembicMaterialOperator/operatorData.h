#pragma once

#include <Alembic/Abc/All.h>
#include "NodeCache.h"

struct OperatorData
{
    Alembic::Abc::IObject materialsObject;
    std::string ns;

    bool useAbcShaders;
    bool useShaderAssignationAttribute;

    std::string shaderAssignationAttribute;

    NodeCollector * createdNodes;

    std::vector<std::pair<std::string, AtNode*> > shaders;
    std::map<std::string, AtNode*> displacements;
    std::map<std::string, std::string> pathRemapping;

};