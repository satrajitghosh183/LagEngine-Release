#include "rldemo/IndirectDrawBuilder.hpp"
#include <glad/glad.h>
#include <cstring>

namespace rldemo {

IndirectDrawBuilder::IndirectDrawBuilder(uint32_t maxInstances)
    : m_MaxInstances(maxInstances) {
    m_InstanceStaging.reserve(maxInstances);
    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

IndirectDrawBuilder::~IndirectDrawBuilder() {
    if (m_InstanceVBO) glDeleteBuffers(1, &m_InstanceVBO);
}

void IndirectDrawBuilder::BeginFrame() {
    std::lock_guard<std::mutex> lock(m_StagingMutex);
    m_InstanceStaging.clear();
}

void IndirectDrawBuilder::PrepareForInstances(uint32_t count) {
    std::lock_guard<std::mutex> lock(m_StagingMutex);
    if (count > m_MaxInstances) count = m_MaxInstances;
    m_InstanceStaging.resize(count);
}

InstanceData* IndirectDrawBuilder::GetInstanceStagingData() {
    return m_InstanceStaging.empty() ? nullptr : m_InstanceStaging.data();
}

void IndirectDrawBuilder::AddInstance(const glm::mat4& model, const glm::vec4& color) {
    std::lock_guard<std::mutex> lock(m_StagingMutex);
    if (m_InstanceStaging.size() >= m_MaxInstances) return;
    InstanceData d;
    memcpy(d.model, &model[0][0], 16 * sizeof(float));
    d.color[0] = color.r;
    d.color[1] = color.g;
    d.color[2] = color.b;
    d.color[3] = color.a;
    m_InstanceStaging.push_back(d);
}

void IndirectDrawBuilder::AddInstances(const InstanceData* data, size_t count) {
    if (!data || count == 0) return;
    std::lock_guard<std::mutex> lock(m_StagingMutex);
    size_t space = m_MaxInstances - m_InstanceStaging.size();
    if (space == 0) return;
    if (count > space) count = space;
    for (size_t i = 0; i < count; ++i)
        m_InstanceStaging.push_back(data[i]);
}

uint32_t IndirectDrawBuilder::EndFrame() {
    std::lock_guard<std::mutex> lock(m_StagingMutex);
    m_DrawCount = static_cast<uint32_t>(m_InstanceStaging.size());
    if (m_DrawCount == 0) return 0;
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_InstanceStaging.size() * sizeof(InstanceData), m_InstanceStaging.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return m_DrawCount;
}

} // namespace rldemo
