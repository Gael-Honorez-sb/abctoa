// Copyright (c) 2014 The Foundry Visionmongers Ltd. All Rights Reserved.

#ifndef ATTRIBUTEDATACACHE_H
#define ATTRIBUTEDATACACHE_H

#include <sstream>
#include <boost/shared_ptr.hpp>

#include <FnGeolib/util/AttributeKeyedCache.h>

#include <FileReader.h>
#include <FileReaderFactory.h>

// This cache maps GroupAttributes containing two StringAttribute children
// (named 'filename' and 'soFilename') to AttrData instances, and performs
// queries (get()) in a thread-safe manner.
class AttrDataCache: public FnGeolibUtil::AttributeKeyedCache<AttrData>
{
public:
    typedef boost::shared_ptr<AttrDataCache> Ptr;

    AttrDataCache() :
        FnGeolibUtil::AttributeKeyedCache<AttrData>(1000, 1000)
    {
    }

private:
    AttrDataCache::IMPLPtr createValue(const FnAttribute::Attribute & iAttr)
    {
        // createValue() is called when there is no previously cached data
        // associated with the given iAttr attribute. Deconstruct the iAttr
        // into filename/soFilename strings, and use the FileReaderFactory
        // to load the attribute data from disk.

        FnAttribute::GroupAttribute groupAttr(iAttr);
        if (!groupAttr.isValid())
        {
            return AttrDataCache::IMPLPtr();
        }

        const std::string filename =
            FnAttribute::StringAttribute(
                groupAttr.getChildByName("filename")).getValue("", false);

        if (filename.empty())
        {
            return AttrDataCache::IMPLPtr();
        }

        // Parse it and add it to the cache
        FileReaderFactory factory;
        FileReaderPtr reader = factory.get();
        const AttrData attrData = reader->read(filename);
        return AttrDataCache::IMPLPtr(new AttrData(attrData));


    }
};

#endif
