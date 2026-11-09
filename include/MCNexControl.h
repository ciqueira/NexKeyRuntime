#ifndef MC_NEXCONTROL_H
#define MC_NEXCONTROL_H

#include "nexcontrol/nexcontrol.h"
#include <string>

namespace mc {
namespace nexcontrol {

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
