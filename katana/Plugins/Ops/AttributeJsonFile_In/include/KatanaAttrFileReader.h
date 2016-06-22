// Copyright (c) 2014 The Foundry Visionmongers Ltd. All Rights Reserved.

#ifndef KATANAATTRFILEREADER_H
#define KATANAATTRFILEREADER_H

#include "FileReader.h"
#include <iostream>
#include <algorithm>

// An implementation of a FileReader that reads an XML file that follows
// Katana's standard for the attribute XML file format.
class KatanaAttrFileReader : public FileReader
{
public:

    AttrData read(std::string filename);
};

#endif

