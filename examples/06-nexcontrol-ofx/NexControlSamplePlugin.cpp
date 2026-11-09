#include "NexControlSamplePlugin.h"
#include <string>
#include <iostream>

namespace OFX {
namespace Private {
extern OfxHost *gHost;
}
}

namespace nexcontrol_sample {

NexControlSamplePlugin::NexControlSamplePlugin(OfxImageEffectHandle handle)
    : OFX::ImageEffect(handle) {

    // 1. Fetch user effect parameters
    m_paramExposure = fetchDoubleParam("exposure");
    m_paramContrast = fetchDoubleParam("contrast");
    m_paramColorMode = fetchChoiceParam("color_mode");
    m_paramBypass = fetchBooleanParam("bypass");
    m_dstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);

    // 2. Initialize the NexControl subsystem (idempotent across multiple plugins/instances)
    mc::nexcontrol::init(OFX::Private::gHost);

    // 3. Allocate an isolated instance binding
    m_nexcontrolBinding = std::make_unique<mc::nexcontrol::InstanceBinding>(handle);

    // 4. Register parameters for MIDI hardware rotary knob control
    // Slot 0: Exposure (Double continuous slider)
    m_nexcontrolBinding->addParam("exposure", NEXCONTROL_PARAM_DOUBLE, -5.0, 5.0, -5.0, 5.0, 0.0);

    // Slot 1: Contrast (Double continuous slider)
    m_nexcontrolBinding->addParam("contrast", NEXCONTROL_PARAM_DOUBLE, 0.0, 2.0, 0.0, 2.0, 1.0);

    // Slot 2: Color Mode Dropdown (Choice / Discrete Step)
    m_nexcontrolBinding->addParam("color_mode", NEXCONTROL_PARAM_CHOICE, 0.0, 2.0, 0.0, 2.0, 0.0);

    // Slot 3: Bypass (Boolean / Toggle switch)
    m_nexcontrolBinding->addParam("bypass", NEXCONTROL_PARAM_BOOL, 0.0, 1.0, 0.0, 1.0, 0.0);

    // 5. Update the status anchor string with the allocated instance ID
    m_paramInstanceInfo = fetchStringParam("instance_info");
    if (m_paramInstanceInfo && m_nexcontrolBinding) {
        std::string beacon = "Node Instance (ID: " +
                             std::to_string(m_nexcontrolBinding->instance_id()) + ")";
        try {
            m_paramInstanceInfo->setValue(beacon);
        } catch (...) {}
    }
}

NexControlSamplePlugin::~NexControlSamplePlugin() {
    // Automatically unregisters the instance slot from the IPC dispatcher
    m_nexcontrolBinding.reset();
}

void NexControlSamplePlugin::render(const OFX::RenderArguments &args) {
    // Standard rendering logic...
}

bool NexControlSamplePlugin::isIdentity(const OFX::IsIdentityArguments &args,
                                       OFX::Clip *&identityClip,
                                       double &identityTime) {
    if (m_paramBypass) {
        bool bypassed = false;
        m_paramBypass->getValueAtTime(args.time, bypassed);
        if (bypassed) {
            identityClip = m_srcClip;
            identityTime = args.time;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Plugin Factory Implementation
// ---------------------------------------------------------------------------

NexControlSamplePluginFactory::NexControlSamplePluginFactory()
    : OFX::PluginFactoryHelper<NexControlSamplePluginFactory>(
          "com.mcnexus.sample.nexcontrol", 1, 0) {}

void NexControlSamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc) {
    desc.setLabels("NexControl Sample Plugin", "NexControl Sample", "NexControl Sample");
    desc.setPluginGrouping("Color");
    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedContext(OFX::eContextGeneral);
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
}

void NexControlSamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc,
                                                     OFX::ContextEnum context) {
    // Source & Destination clips
    OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGB);

    OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGB);

    // Main page
    OFX::PageParamDescriptor *page = desc.definePageParam("Controls");

    // 1. Exposure Slider
    OFX::DoubleParamDescriptor *paramExposure = desc.defineDoubleParam("exposure");
    paramExposure->setLabels("Exposure", "Exposure", "Exposure");
    paramExposure->setRange(-5.0, 5.0);
    paramExposure->setDisplayRange(-5.0, 5.0);
    paramExposure->setDefault(0.0);
    page->addChild(*paramExposure);

    // 2. Contrast Slider
    OFX::DoubleParamDescriptor *paramContrast = desc.defineDoubleParam("contrast");
    paramContrast->setLabels("Contrast", "Contrast", "Contrast");
    paramContrast->setRange(0.0, 2.0);
    paramContrast->setDisplayRange(0.0, 2.0);
    paramContrast->setDefault(1.0);
    page->addChild(*paramContrast);

    // 3. Color Mode Dropdown
    OFX::ChoiceParamDescriptor *paramMode = desc.defineChoiceParam("color_mode");
    paramMode->setLabels("Color Mode", "Color Mode", "Color Mode");
    paramMode->appendOption("Rec.709");
    paramMode->appendOption("ACEScc");
    paramMode->appendOption("DaVinci Wide Gamut");
    paramMode->setDefault(0);
    page->addChild(*paramMode);

    // 4. Bypass Toggle
    OFX::BooleanParamDescriptor *paramBypass = desc.defineBooleanParam("bypass");
    paramBypass->setLabels("Bypass", "Bypass", "Bypass");
    paramBypass->setDefault(false);
    page->addChild(*paramBypass);

    // 5. Dedicated NexControl Group & Anchor Parameter
    OFX::GroupParamDescriptor *grpNexControl = desc.defineGroupParam("grpNexControl");
    grpNexControl->setLabels("NexControl", "NexControl", "NexControl");
    grpNexControl->setOpen(false); // Closed by default
    page->addChild(*grpNexControl);

    OFX::StringParamDescriptor *instParam = desc.defineStringParam("instance_info");
    instParam->setLabels("Status", "Status", "Status");
    instParam->setStringType(OFX::eStringTypeSingleLine);
    instParam->setDefault("Node Instance (Inactive)");
    instParam->setEnabled(false);       // Read-only indicator
    instParam->setIsPersistant(false);   // Critical: Do NOT save to project .drp files
    instParam->setParent(*grpNexControl);
    page->addChild(*instParam);
}

OFX::ImageEffect *NexControlSamplePluginFactory::createInstance(OfxImageEffectHandle handle,
                                                               OFX::ContextEnum context) {
    return new NexControlSamplePlugin(handle);
}

} // namespace nexcontrol_sample

// OpenFX Plugin Entry Point
namespace OFX {
namespace Plugin {
void getPluginIDs(OFX::PluginFactoryArray &ids) {
    static nexcontrol_sample::NexControlSamplePluginFactory samplePluginFactory;
    ids.push_back(&samplePluginFactory);
}
} // namespace Plugin
} // namespace OFX
