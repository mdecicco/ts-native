#include <utils/PeriodicUpdate.h>
#include <utils/List.hpp>

namespace utils {
    IPeriodicUpdate::IPeriodicUpdate() : m_targetInterval(1.0f / 60.0f), m_parent(nullptr) {
    }

    IPeriodicUpdate::~IPeriodicUpdate() {
    }

    void IPeriodicUpdate::setUpdateFrequency(f32 frequency) {
        m_targetInterval = 1.0f / frequency;
    }

    f32 IPeriodicUpdate::getUpdateFrequency() const {
        return 1.0f / m_targetInterval;
    }

    void IPeriodicUpdate::setUpdateInterval(f32 interval) {
        m_targetInterval = interval;
    }

    f32 IPeriodicUpdate::getUpdateInterval() const {
        return m_targetInterval;
    }

    void IPeriodicUpdate::enableUpdates() {
        if (!m_updateTimer.stopped()) return;
        m_updateTimer.reset();
        m_updateTimer.start();
        onUpdatesEnabled();
    }

    void IPeriodicUpdate::disableUpdates() {
        if (m_updateTimer.stopped()) return;
        m_updateTimer.pause();
        onUpdatesDisabled();
    }

    void IPeriodicUpdate::maybeUpdate(f32 deltaTime) {
        for (IPeriodicUpdate* child : m_children) child->maybeUpdate(deltaTime);

        if (m_updateTimer.stopped() || m_updateTimer < m_targetInterval) return;
        onUpdate(deltaTime, m_updateTimer);
        m_updateTimer.reset();
        m_updateTimer.start();
    }

    void IPeriodicUpdate::addChild(IPeriodicUpdate* child) {
        if (child->m_parent) {
            if (child->m_parent == this) return;
            child->m_parent->removeChild(child);
        }

        m_children.push(child);
        child->m_parent = this;
    }

    void IPeriodicUpdate::removeChild(IPeriodicUpdate* child) {
        if (child->m_parent != this) return;
        auto node = m_children.findNode([child](IPeriodicUpdate* c) { return c == child; });
        m_children.remove(node);
        child->m_parent = nullptr;
    }

    u32 IPeriodicUpdate::getChildCount() const {
        return m_children.size();
    }

    void IPeriodicUpdate::onUpdatesEnabled() {
    }

    void IPeriodicUpdate::onUpdatesDisabled() {
    }

    void IPeriodicUpdate::onUpdate(f32 frameDelta, f32 updateDelta) {
    }
};