#pragma once

#include "ofxsImageEffect.h"
#include "MCNexControl.h"
#include <memory>

namespace nexcontrol_sample {

class NexControlSamplePlugin : public OFX::ImageEffect {
public:
    explicit NexControlSamplePlugin(OfxImageEffectHandle handle);
    virtual ~NexControlSamplePlugin();

    // OpenFX mandatory overrides
    virtual void render(const OFX::RenderArguments &args) override;
    virtual bool isIdentity(const OFX::IsIdentityArguments &args,
                            OFX::Clip *&identityClip,
                            double &identityTime) override;

private:
    // User effect parameters
    OFX::DoubleParam *m_paramExposure = nullptr;
    OFX::DoubleParam *m_paramContrast = nullptr;
    OFX::ChoiceParam *m_paramColorMode = nullptr;
    OFX::BooleanParam *m_paramBypass = nullptr;

    // NexControl status anchor parameter (Required for node identification)
    OFX::StringParam *m_paramInstanceInfo = nullptr;

    // Instance binding lifecycle holder
    std::unique_ptr<mc::nexcontrol::InstanceBinding> m_nexcontrolBinding;

    // Input and output image clips
    OFX::Clip *m_dstClip = nullptr;
    OFX::Clip *m_srcClip = nullptr;
};

class NexControlSamplePluginFactory : public OFX::PluginFactoryHelper<NexControlSamplePluginFactory> {
public:
    NexControlSamplePluginFactory();
    virtual void describe(OFX::ImageEffectDescriptor &desc) override;
    virtual void describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context) override;
    virtual OFX::ImageEffect *createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context) override;
};

} // namespace nexcontrol_sample
