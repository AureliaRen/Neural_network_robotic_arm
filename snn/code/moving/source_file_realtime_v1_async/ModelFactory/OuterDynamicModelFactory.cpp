#include "../source_file_realtime_v1_async/ModelFactory/OuterDynamicModelFactory.h"

#if SNN_WITH_PINOCCHIO
#include "../source_file_realtime_v1_async/OuterDynamic/inc/PlanarArm2DOFPinocchio.h"
#endif
#include "../source_file_realtime_v1_async/OuterDynamic/inc/TotalModelPlanarArm2DOF.h"

#include <stdexcept>

OuterDynamicModel* OuterDynamicModelFactory::createOuterDynamicModel(const OuterDynamicDescription& description) {
#if SNN_WITH_PINOCCHIO
    if (description.ModelName == "PlanarArm2DOFPinocchio") {
        return new PlanarArm2DOFPinocchio(description.update_timestep);
    }
#endif
    if (description.ModelName == "TotalModelPlanarArm2DOF") {
        return new TotalModelPlanarArm2DOF(description.update_timestep);
    }
    throw std::runtime_error("Unknown or unavailable outer dynamic model: " + description.ModelName);
}
