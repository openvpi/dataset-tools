#include <dsfw/PassthroughProcessor.h>
#include <dsfw/TaskProcessorRegistry.h>

namespace dstools {

QString PassthroughProcessor::processorId() const {
    return QStringLiteral("passthrough");
}

QString PassthroughProcessor::displayName() const {
    return QStringLiteral("Passthrough");
}

TaskSpec PassthroughProcessor::taskSpec() const {
    return {QStringLiteral("passthrough"), {}, {}};
}

Result<void> PassthroughProcessor::initialize(IModelManager &, const ProcessorConfig &) {
    return Result<void>::Ok();
}

void PassthroughProcessor::release() {
}

Result<TaskOutput> PassthroughProcessor::process(const TaskInput &input) {
    TaskOutput output;
    output.layers = input.layers;
    return Result<TaskOutput>::Ok(std::move(output));
}

static TaskProcessorRegistry::Registrar<PassthroughProcessor> s_passthroughReg(
    QStringLiteral("passthrough"), QStringLiteral("passthrough"));

} // namespace dstools
