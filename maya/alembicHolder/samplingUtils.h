#ifndef SAMPLING_UTIL_H_
#define SAMPLING_UTIL_H_

#include <Alembic/AbcGeom/All.h>

double getWeightAndIndex(double iFrame,
    Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps,
    Alembic::AbcCoreAbstract::index_t & oIndex,
    Alembic::AbcCoreAbstract::index_t & oCeilIndex);

template<typename T>
T simpleLerp(double alpha, T val1, T val2)
{
    double dv = static_cast<double>( val1 );
    return static_cast<T>( dv + alpha * (static_cast<double>(val2) - dv) );
}

#endif