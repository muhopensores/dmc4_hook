#pragma once

#include "cUnit.hpp"
#include "MtMath.hpp"

class uCoord: public cUnit {
public:
    uCoord* mpParent;
    int ParentJoint; //attached entity's joint index
    int mOrder; //axis order
    int padding24[3];
    MtVector3 mPos;
    MtVector4 mQuat;
    MtVector3 mScale;
    MtMatrix mLmat;
    MtMatrix mWmat;
};

static_assert(sizeof(uCoord) == 0xe0);
