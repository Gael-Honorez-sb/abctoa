#include "samplingUtils.h"

double getWeightAndIndex(double iFrame,
    Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps,
    Alembic::AbcCoreAbstract::index_t & oIndex,
    Alembic::AbcCoreAbstract::index_t & oCeilIndex)
{
    if (numSamps == 0)
        numSamps = 1;

    std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
        iTime->getFloorIndex(iFrame, numSamps);

    oIndex = floorIndex.first;
    oCeilIndex = oIndex;

    if (fabs(iFrame - floorIndex.second) < 0.0001)
        return 0.0;

    std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
        iTime->getCeilIndex(iFrame, numSamps);

    if (oIndex == ceilIndex.first)
        return 0.0;

    oCeilIndex = ceilIndex.first;

    double alpha = (iFrame - floorIndex.second) /
        (ceilIndex.second - floorIndex.second);

    // we so closely match the ceiling so we'll just use it
    if (fabs(1.0 - alpha) < 0.0001)
    {
        oIndex = oCeilIndex;
        return 0.0;
    }

    return alpha;
}