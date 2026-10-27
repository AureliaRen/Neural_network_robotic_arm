#ifndef OUTER_DYNAMIC_MODEL_FACTORY_H
#define OUTER_DYNAMIC_MODEL_FACTORY_H

#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicModel.h"

class OuterDynamicModelFactory {
public:
    static OuterDynamicModel* createOuterDynamicModel(const OuterDynamicDescription& description);
};

#endif
