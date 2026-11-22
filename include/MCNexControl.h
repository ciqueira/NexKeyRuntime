#ifndef MC_NEXCONTROL_H
#define MC_NEXCONTROL_H

#include "nexcontrol/nexcontrol.h"
#include <string>

namespace mc {
namespace nexcontrol {

enum class GroupLayout : uint8_t {
    AutoPack      = 0,
    DedicatedBank = 1
};

class BatchScope {
public:
    explicit BatchScope(uint32_t instanceId) noexcept : instanceId_(instanceId) {
        if (instanceId_ != 0) {
            nexcontrol_begin_batch(instanceId_);
        }
    }

    ~BatchScope() {
        if (instanceId_ != 0) {
            nexcontrol_end_batch(instanceId_);
            instanceId_ = 0;
        }
    }

    BatchScope(const BatchScope&) = delete;
    BatchScope& operator=(const BatchScope&) = delete;

    BatchScope(BatchScope&& other) noexcept : instanceId_(other.instanceId_) {
        other.instanceId_ = 0;
    }

    BatchScope& operator=(BatchScope&& other) noexcept {
        if (this != &other) {
            if (instanceId_ != 0) {
                nexcontrol_end_batch(instanceId_);
            }
            instanceId_ = other.instanceId_;
            other.instanceId_ = 0;
        }
        return *this;
    }

    bool isBound() const noexcept { return instanceId_ != 0; }

private:
    uint32_t instanceId_;
};

class GroupScope {
public:
    GroupScope(uint32_t instanceId, const char* groupLabel, GroupLayout layout) noexcept
        : instanceId_(instanceId)
        , groupLabel_(groupLabel)
        , layout_(layout) {}

    ~GroupScope() = default;

    GroupScope(const GroupScope&) = delete;
    GroupScope& operator=(const GroupScope&) = delete;

    GroupScope(GroupScope&& other) noexcept
        : instanceId_(other.instanceId_)
        , groupLabel_(other.groupLabel_)
        , layout_(other.layout_) {
        other.instanceId_ = 0;
        other.groupLabel_ = nullptr;
    }

    GroupScope& operator=(GroupScope&& other) noexcept {
        if (this != &other) {
            instanceId_ = other.instanceId_;
            groupLabel_ = other.groupLabel_;
            layout_     = other.layout_;
            other.instanceId_ = 0;
            other.groupLabel_ = nullptr;
        }
        return *this;
    }

    GroupScope& addParam(
        const char* scriptName,
        uint8_t paramType,
        double minVal,
        double maxVal,
        double displayMin,
        double displayMax,
        double defaultVal,
        bool pinnedInSubpages = false,
        uint8_t extraFlags = 0) noexcept {
        if (instanceId_ != 0) {
            const uint8_t flags = static_cast<uint8_t>(
                (layout_ == GroupLayout::DedicatedBank ? NEXCONTROL_PARAM_FLAG_GROUP_DEDICATED : 0) |
                (pinnedInSubpages ? NEXCONTROL_PARAM_FLAG_PINNED_SUBPAGES : 0) |
                extraFlags);
            nexcontrol_add_param_ex(
                instanceId_,
                scriptName,
                paramType,
                minVal,
                maxVal,
                displayMin,
                displayMax,
                defaultVal,
                groupLabel_,
                flags);
        }
        return *this;
    }

    GroupScope& addParam(
        const char* scriptName,
        NexControlParamType paramType,
        double minVal,
        double maxVal,
        double displayMin,
        double displayMax,
        double defaultVal,
        bool pinnedInSubpages = false,
        uint8_t extraFlags = 0) noexcept {
        return addParam(
            scriptName,
            static_cast<uint8_t>(paramType),
            minVal,
            maxVal,
            displayMin,
            displayMax,
            defaultVal,
            pinnedInSubpages,
            extraFlags);
    }

    bool isBound() const noexcept { return instanceId_ != 0; }

private:
    uint32_t    instanceId_;
    const char* groupLabel_;
    GroupLayout layout_;
};

class InstanceBinding {
public:
    InstanceBinding() : instanceId_(0) {}

    explicit InstanceBinding(OfxImageEffectHandle instance) : instanceId_(0) {
        bind(instance);
    }

    ~InstanceBinding() {
        unbind();
    }

    InstanceBinding(const InstanceBinding&) = delete;
    InstanceBinding& operator=(const InstanceBinding&) = delete;

    InstanceBinding(InstanceBinding&& other) noexcept : instanceId_(other.instanceId_) {
        other.instanceId_ = 0;
    }

    InstanceBinding& operator=(InstanceBinding&& other) noexcept {
        if (this != &other) {
            unbind();
            instanceId_ = other.instanceId_;
            other.instanceId_ = 0;
        }
        return *this;
    }

    bool bind(OfxImageEffectHandle instance) {
        if (instanceId_ != 0) {
            unbind();
        }
        if (!instance) return false;
        uint32_t id = 0;
        if (nexcontrol_register_instance(instance, &id) == NEXCONTROL_OK) {
            instanceId_ = id;
            return true;
        }
        return false;
    }

    void unbind() {
        if (instanceId_ != 0) {
            nexcontrol_unregister_instance(instanceId_);
            instanceId_ = 0;
        }
    }

    void setFocus(bool focused) {
        if (instanceId_ != 0) {
            nexcontrol_set_focus(instanceId_, focused);
        }
    }

    bool addParam(const char* scriptName, uint8_t paramType, double minVal, double maxVal, double displayMin, double displayMax, double defaultVal) {
        if (instanceId_ == 0) return false;
        return nexcontrol_add_param(instanceId_, scriptName, paramType, minVal, maxVal, displayMin, displayMax, defaultVal) == NEXCONTROL_OK;
    }

    bool addParam(const char* scriptName, NexControlParamType paramType, double minVal, double maxVal, double displayMin, double displayMax, double defaultVal) {
        return addParam(scriptName, static_cast<uint8_t>(paramType), minVal, maxVal, displayMin, displayMax, defaultVal);
    }

    BatchScope beginBatch() const noexcept {
        return BatchScope(instanceId_);
    }

    GroupScope beginGroup(const char* groupLabel, GroupLayout layout = GroupLayout::AutoPack) const noexcept {
        return GroupScope(instanceId_, groupLabel, layout);
    }

    bool notifyUserEdit(const char* scriptName) noexcept {
        if (instanceId_ == 0 || !scriptName) {
            return false;
        }
        return nexcontrol_notify_user_edit(instanceId_, scriptName) == NEXCONTROL_OK;
    }

    uint32_t id() const noexcept { return instanceId_; }
    uint32_t instance_id() const noexcept { return instanceId_; }
    bool isBound() const noexcept { return instanceId_ != 0; }

private:
    uint32_t instanceId_;
};

inline NexControlResult init(OfxHost* host, NexKeyRuntimeLicenseHandle* licenseHandle = nullptr) {
    return nexcontrol_init(host, licenseHandle);
}

inline NexControlResult shutdown() {
    return nexcontrol_shutdown();
}

inline NexControlResult defineParamPriority(const std::string& scriptName, uint16_t prioritySlot) {
    return nexcontrol_define_param_priority(scriptName.c_str(), prioritySlot);
}

} // namespace nexcontrol
} // namespace mc

#endif // MC_NEXCONTROL_H
