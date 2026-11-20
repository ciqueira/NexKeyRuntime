#include "NexControlSamplePlugin.h"
#include "ofxsMultiThread.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace OFX {
namespace Private {
extern OfxHost *gHost;
}
}

namespace nexcontrol_sample {

namespace {

class NexControlPixelProcessor : public OFX::MultiThread::Processor {
public:
    NexControlPixelProcessor(OFX::ImageEffect &effect,
                             const OFX::Image *srcImg,
                             OFX::Image *dstImg,
                             OfxRectI window,
                             int nComps,
                             bool bypass,
                             float gain,
                             float contrast,
                             float pivot)
        : effect_(effect),
          srcImg_(srcImg),
          dstImg_(dstImg),
          window_(window),
          nComps_(nComps),
          bypass_(bypass),
          gain_(gain),
          contrast_(contrast),
          pivot_(pivot) {}

    void multiThreadFunction(unsigned int threadId, unsigned int nThreads) override {
        if (nThreads == 0 || window_.y2 <= window_.y1 || window_.x2 <= window_.x1) {
            return;
        }

        const unsigned int dy = static_cast<unsigned int>(window_.y2 - window_.y1);
        const unsigned int h = (dy + nThreads - 1u) / nThreads;
        if (threadId * h >= dy) {
            return;
        }

        const int yStart = window_.y1 + static_cast<int>(threadId * h);
        const int yEnd = window_.y1 + static_cast<int>(std::min(dy, (threadId + 1u) * h));
        const OfxRectI srcBounds = srcImg_->getBounds();
        const int rowWidth = window_.x2 - window_.x1;

        for (int y = yStart; y < yEnd; ++y) {
            if (effect_.abort()) {
                break;
            }

            float *dstRow = static_cast<float *>(dstImg_->getPixelAddress(window_.x1, y));
            if (!dstRow) {
                continue;
            }

            const int xStart = std::max(window_.x1, srcBounds.x1);
            const int xEnd = std::min(window_.x2, srcBounds.x2);
            if (y < srcBounds.y1 || y >= srcBounds.y2 || xStart >= xEnd) {
                zeroFill(dstRow, rowWidth);
                continue;
            }

            if (xStart > window_.x1) {
                zeroFill(dstRow, xStart - window_.x1);
            }
            if (xEnd < window_.x2) {
                zeroFill(dstRow + static_cast<std::ptrdiff_t>(xEnd - window_.x1) * nComps_,
                         window_.x2 - xEnd);
            }

            const float *srcPtr = static_cast<const float *>(srcImg_->getPixelAddress(xStart, y));
            float *dstPtr = dstRow + static_cast<std::ptrdiff_t>(xStart - window_.x1) * nComps_;
            const int spanWidth = xEnd - xStart;
            if (!srcPtr) {
                zeroFill(dstPtr, spanWidth);
                continue;
            }

            for (int x = 0; x < spanWidth; ++x) {
                if (bypass_) {
                    std::copy_n(srcPtr, static_cast<std::size_t>(nComps_), dstPtr);
                } else {
                    dstPtr[0] = (srcPtr[0] * gain_ - pivot_) * contrast_ + pivot_;
                    dstPtr[1] = (srcPtr[1] * gain_ - pivot_) * contrast_ + pivot_;
                    dstPtr[2] = (srcPtr[2] * gain_ - pivot_) * contrast_ + pivot_;
                    if (nComps_ == 4) {
                        dstPtr[3] = srcPtr[3];
                    }
                }
                srcPtr += nComps_;
                dstPtr += nComps_;
            }
        }
    }

private:
    void zeroFill(float *dst, int pixelCount) const {
        std::fill_n(dst,
                    static_cast<std::size_t>(pixelCount) * static_cast<std::size_t>(nComps_),
                    0.0f);
    }

    OFX::ImageEffect &effect_;
    const OFX::Image *srcImg_;
    OFX::Image *dstImg_;
    OfxRectI window_;
    int nComps_;
    bool bypass_;
    float gain_;
    float contrast_;
    float pivot_;
};

} // namespace

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
    if (!m_srcClip || !m_dstClip) {
        return;
    }

    std::unique_ptr<OFX::Image> dstImg(m_dstClip->fetchImage(args.time));
    std::unique_ptr<OFX::Image> srcImg(m_srcClip->fetchImage(args.time));
    if (!dstImg || !srcImg) {
        return;
    }

    if (dstImg->getPixelDepth() != OFX::eBitDepthFloat ||
        srcImg->getPixelDepth() != OFX::eBitDepthFloat) {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    const int nComps = dstImg->getPixelComponentCount();
    if ((nComps != 3 && nComps != 4) || srcImg->getPixelComponentCount() != nComps) {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    bool bypassed = false;
    if (m_paramBypass) {
        m_paramBypass->getValueAtTime(args.time, bypassed);
    }

    double exposure = 0.0;
    if (m_paramExposure) {
        m_paramExposure->getValueAtTime(args.time, exposure);
    }

    double contrast = 1.0;
    if (m_paramContrast) {
        m_paramContrast->getValueAtTime(args.time, contrast);
    }

    int colorMode = 0;
    if (m_paramColorMode) {
        m_paramColorMode->getValueAtTime(args.time, colorMode);
    }

    const float gain = static_cast<float>(std::pow(2.0, exposure));
    const float contrastF = static_cast<float>(contrast);
    float pivot = 0.5f; // Rec.709 default
    if (colorMode == 1) {
        pivot = 0.4135884f; // ACEScc 18% gray
    } else if (colorMode == 2) {
        pivot = 0.336043f; // DaVinci Wide Gamut / Intermediate 18% gray
    }

    const OfxRectI dstBounds = dstImg->getBounds();
    OfxRectI win;
    win.x1 = std::max(args.renderWindow.x1, dstBounds.x1);
    win.y1 = std::max(args.renderWindow.y1, dstBounds.y1);
    win.x2 = std::min(args.renderWindow.x2, dstBounds.x2);
    win.y2 = std::min(args.renderWindow.y2, dstBounds.y2);
    if (win.x1 >= win.x2 || win.y1 >= win.y2) {
        return;
    }

    const unsigned int width = static_cast<unsigned int>(std::min(win.x2 - win.x1, 4096));
    const unsigned int height = static_cast<unsigned int>(win.y2 - win.y1);
    unsigned int nCPUs = (width * height) / 4096u;
    nCPUs = std::max(1u, std::min(nCPUs, OFX::MultiThread::getNumCPUs()));

    NexControlPixelProcessor processor(*this,
                                       srcImg.get(),
                                       dstImg.get(),
                                       win,
                                       nComps,
                                       bypassed,
                                       gain,
                                       contrastF,
                                       pivot);
    processor.multiThread(nCPUs);
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
    instParam->setEvaluateOnChange(false);
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
