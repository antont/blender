#include "COM_RenderLayersEnvironmentOperation.h"

RenderLayersEnvironmentOperation::RenderLayersEnvironmentOperation() :RenderLayersBaseProg(SCE_PASS_ENVIRONMENT, 3) {
    this->addOutputSocket(COM_DT_COLOR);
}
