// =============================================================================
// NexKeyRuntime Sandbox: Clean Reference OFX Plugin (Profile A Showcase)
// -----------------------------------------------------------------------------
// Integrated showcase implementation.
// =============================================================================

#include "CleanReferencePlugin.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

static int platformOpenUrl(const char *url) {
    if (!url || url[0] == '\0') return 0;
#if defined(_WIN32)
    HINSTANCE res = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    return (reinterpret_cast<INT_PTR>(res) > 32) ? 1 : 0;
#elif defined(__APPLE__)
    std::string cmd = "open \"" + std::string(url) + "\" &";
    return std::system(cmd.c_str()) == 0 ? 1 : 0;
#else
    std::string cmd = "xdg-open \"" + std::string(url) + "\" &";
    return std::system(cmd.c_str()) == 0 ? 1 : 0;
#endif
}

// ─── Clean CPU Pixel Processor ───────────────────────────────────────────────

class CleanPixelProcessor : public OFX::ImageProcessor {
public:
    CleanPixelProcessor(OFX::ImageEffect &instance,
                        OFX::Image *src,
                        OFX::Image *dst,
                        bool allowed,
                        float gain,
                        bool invert,
                        bool demoWatermark)
        : OFX::ImageProcessor(instance),
          srcImg_(src),
          dstImg_(dst),
          allowed_(allowed),
          gain_(gain),
          invert_(invert),
          watermark_(demoWatermark) {}

    virtual void multiThreadProcessImages(OfxRectI window) override {
        for (int y = window.y1; y < window.y2; ++y) {
            const float *srcRow = static_cast<const float *>(srcImg_->getPixelAddress(window.x1, y));
            float *dstRow = static_cast<float *>(dstImg_->getPixelAddress(window.x1, y));

            if (!srcRow || !dstRow) continue;

            const int width = window.x2 - window.x1;
            for (int x = 0; x < width; ++x) {
                const int globalX = window.x1 + x;
                const int globalY = y;
                const int idx = x * 4;

                if (!allowed_) {
                    // Unlicensed / Denied: pure passthrough (effect cleanly bypassed)
                    dstRow[idx + 0] = srcRow[idx + 0];
                    dstRow[idx + 1] = srcRow[idx + 1];
                    dstRow[idx + 2] = srcRow[idx + 2];
                    dstRow[idx + 3] = srcRow[idx + 3];
                    continue;
                }

                // Apply exposure adjustment
                float r = srcRow[idx + 0] * gain_;
                float g = srcRow[idx + 1] * gain_;
                float b = srcRow[idx + 2] * gain_;
                float a = srcRow[idx + 3];

                // Apply color inversion if enabled
                if (invert_) {
                    r = 1.0f - r;
                    g = 1.0f - g;
                    b = 1.0f - b;
                }

                // Demo Edition: overlay subtle diagonal watermark pattern on CPU
                if (watermark_) {
                    if (((globalX + globalY) / 24) % 8 == 0) {
                        r = r * 0.7f + 0.3f;
                        g = g * 0.7f + 0.3f;
                        b = b * 0.7f + 0.3f;
                    }
                }

                dstRow[idx + 0] = r;
                dstRow[idx + 1] = g;
                dstRow[idx + 2] = b;
                dstRow[idx + 3] = a;
            }
        }
    }

private:
    OFX::Image *srcImg_;
    OFX::Image *dstImg_;
    bool allowed_;
    float gain_;
    bool invert_;
    bool watermark_;
};

// ─── OFX Plugin Instance ─────────────────────────────────────────────────────

class CleanReferencePlugin : public OFX::ImageEffect {
public:
    explicit CleanReferencePlugin(OfxImageEffectHandle handle);
    virtual ~CleanReferencePlugin() override;

    virtual void render(const OFX::RenderArguments &args) override;
    virtual bool isIdentity(const OFX::IsIdentityArguments &args,
                            OFX::Clip *&identityClip,
                            double &identityTime) override;
    virtual void changedParam(const OFX::InstanceChangedArgs &args,
                              const std::string &paramName) override;

private:
    void updateLicenseTelemetry();
    void updateNoticesUI();

    OFX::Clip *srcClip_;
    OFX::Clip *dstClip_;

    OFX::DoubleParam *paramExposure_;
    OFX::BooleanParam *paramInvert_;

    // Licensing diagnostics
    OFX::StringParam *diagVersion_;
    OFX::StringParam *diagStatus_;
    OFX::StringParam *diagEdition_;
    OFX::StringParam *diagDecision_;
    OFX::StringParam *diagReceiptsPath_;
    OFX::PushButtonParam *btnRefresh_;

    // Updates & notices
    OFX::StringParam *paramUpdateStatus_;
    OFX::PushButtonParam *btnCheckUpdates_;
    OFX::StringParam *paramNoticeBanner_;
    OFX::PushButtonParam *btnDismissNotice_;
    OFX::PushButtonParam *btnOpenAction_;

    mc::License license_;
    mc::Notices notices_;
};

CleanReferencePlugin::CleanReferencePlugin(OfxImageEffectHandle handle)
    : OFX::ImageEffect(handle),
      srcClip_(nullptr),
      dstClip_(nullptr),
      paramExposure_(nullptr),
      paramInvert_(nullptr),
      diagVersion_(nullptr),
      diagStatus_(nullptr),
      diagEdition_(nullptr),
      diagDecision_(nullptr),
      diagReceiptsPath_(nullptr),
      btnRefresh_(nullptr),
      paramUpdateStatus_(nullptr),
      btnCheckUpdates_(nullptr),
      paramNoticeBanner_(nullptr),
      btnDismissNotice_(nullptr),
      btnOpenAction_(nullptr)
{
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);

    paramExposure_ = fetchDoubleParam(kParamExposure);
    paramInvert_ = fetchBooleanParam(kParamInvert);

    diagVersion_ = fetchStringParam(kParamDiagVersion);
    diagStatus_ = fetchStringParam(kParamDiagStatus);
    diagEdition_ = fetchStringParam(kParamDiagEdition);
    diagDecision_ = fetchStringParam(kParamDiagRenderDecision);
    diagReceiptsPath_ = fetchStringParam(kParamDiagReceiptsPath);
    btnRefresh_ = fetchPushButtonParam(kParamBtnRefresh);

    paramUpdateStatus_ = fetchStringParam(kParamUpdateStatus);
    btnCheckUpdates_ = fetchPushButtonParam(kParamBtnCheckUpdates);
    paramNoticeBanner_ = fetchStringParam(kParamNoticeBanner);
    btnDismissNotice_ = fetchPushButtonParam(kParamBtnDismissNotice);
    btnOpenAction_ = fetchPushButtonParam(kParamBtnOpenAction);

    // 1. Initialize NexKeyRuntime Profile A offline licensing
    license_.start();

    // 2. Initialize process-wide NexKeyRuntime Updates and Notices
    notices_.start(platformOpenUrl, "DaVinciResolve", "20.0");

    updateLicenseTelemetry();
    updateNoticesUI();
}

CleanReferencePlugin::~CleanReferencePlugin() {
    license_.shutdown();
    notices_.shutdown();
}

void CleanReferencePlugin::updateLicenseTelemetry() {
    mc::LicenseReport report = license_.report();

    if (diagVersion_) diagVersion_->setValue(kPluginVersion);
    if (diagStatus_) diagStatus_->setValue(report.status.c_str());

    std::string editionStr = report.edition;
    if (license_.daysRemaining() > 0) {
        editionStr += " (" + std::to_string(license_.daysRemaining()) + " days remaining)";
    }
    if (diagEdition_) diagEdition_->setValue(editionStr.c_str());

    if (diagDecision_) {
        diagDecision_->setValue(license_.allowed() ? "ALLOW (< 1 ns atomic check)" : "DENY (Unlicensed)");
    }
    if (diagReceiptsPath_) diagReceiptsPath_->setValue(report.receiptsDir.c_str());
}

void CleanReferencePlugin::updateNoticesUI() {
    mc::NoticeView view = notices_.view();

    if (paramUpdateStatus_) {
        if (view.offerUpdate) {
            paramUpdateStatus_->setValue(("UPDATE AVAILABLE: " + view.text).c_str());
        } else {
            paramUpdateStatus_->setValue(("v" kPluginVersion " (Up to date)"));
        }
    }

    if (paramNoticeBanner_) {
        if (view.visible && !view.offerUpdate) {
            paramNoticeBanner_->setValue(view.text.c_str());
        } else {
            paramNoticeBanner_->setValue("No active vendor notices for this host.");
        }
    }
}

void CleanReferencePlugin::changedParam(const OFX::InstanceChangedArgs &args,
                                        const std::string &paramName)
{
    (void)args;
    if (paramName == kParamBtnRefresh) {
        license_.refresh();
        updateLicenseTelemetry();
    } else if (paramName == kParamBtnCheckUpdates) {
        notices_.refresh();
        updateNoticesUI();
    } else if (paramName == kParamBtnDismissNotice) {
        notices_.dismissActiveNotice();
        updateNoticesUI();
    } else if (paramName == kParamBtnOpenAction) {
        notices_.openAction();
    }
}

bool CleanReferencePlugin::isIdentity(const OFX::IsIdentityArguments &args,
                                      OFX::Clip *&identityClip,
                                      double &identityTime)
{
    // Fast path: if license denies render, bypass
    if (!license_.allowed()) {
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }

    const double exposure = paramExposure_ ? paramExposure_->getValueAtTime(args.time) : 0.0;
    const bool invert = paramInvert_ ? paramInvert_->getValueAtTime(args.time) : false;

    // In demo mode we cannot identity-bypass because watermark must be drawn
    if (exposure == 0.0 && !invert && !license_.demoWatermark()) {
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }

    return false;
}

void CleanReferencePlugin::render(const OFX::RenderArguments &args) {
    if (!srcClip_ || !dstClip_) return;

    std::unique_ptr<OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));

    if (!srcImg || !dstImg) return;

    // ── HOT PATH: Atomic < 1 ns render check ─────────────────────────────────
    const bool allowed = license_.allowed();
    const bool demoWatermark = license_.demoWatermark();

    const double exposure = paramExposure_ ? paramExposure_->getValueAtTime(args.time) : 0.0;
    const bool invert = paramInvert_ ? paramInvert_->getValueAtTime(args.time) : false;
    const float gain = static_cast<float>(std::pow(2.0, exposure));

    CleanPixelProcessor processor(*this,
                                  srcImg.get(),
                                  dstImg.get(),
                                  allowed,
                                  gain,
                                  invert,
                                  demoWatermark);

    processor.setRenderWindow(args.renderWindow);
    processor.process();
}

// ─── Plugin Factory ──────────────────────────────────────────────────────────

CleanReferencePluginFactory::CleanReferencePluginFactory()
    : OFX::PluginFactoryHelper<CleanReferencePluginFactory>(kPluginIdentifier,
                                                           kPluginVersionMajor,
                                                           kPluginVersionMinor) {}

void CleanReferencePluginFactory::describe(OFX::ImageEffectDescriptor &desc) {
    desc.setLabels("NexKey Sandbox Plugin",
                   "NexKey Sandbox",
                   "NexKey Sandbox Plugin");
    desc.setPluginGrouping("MCNexus");
    desc.setPluginDescription("Clean reference OpenFX plugin demonstrating complete NexKeyRuntime integration (Licensing, Watermarks, Updates, Notices).");

    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedContext(OFX::eContextGeneral);

    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(false);
    desc.setSupportsTiles(false);
}

void CleanReferencePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc,
                                                    OFX::ContextEnum context)
{
    (void)context;
    OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentAlpha);

    OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentAlpha);

    // 1. Exposure adjustment
    OFX::DoubleParamDescriptor *paramExposure = desc.defineDoubleParam(kParamExposure);
    paramExposure->setLabels(kParamExposureLabel, kParamExposureLabel, kParamExposureLabel);
    paramExposure->setDefault(0.0);
    paramExposure->setRange(-5.0, 5.0);
    paramExposure->setDisplayRange(-3.0, 3.0);

    // 2. Invert colors toggle
    OFX::BooleanParamDescriptor *paramInvert = desc.defineBooleanParam(kParamInvert);
    paramInvert->setLabels(kParamInvertLabel, kParamInvertLabel, kParamInvertLabel);
    paramInvert->setDefault(false);

    // 3. License diagnostics parameter group
    OFX::GroupParamDescriptor *diagGroup = desc.defineGroupParam(kGroupDiagnostics);
    diagGroup->setLabels("NexKey License Diagnostics",
                         "NexKey License",
                         "NexKey License Diagnostics");
    diagGroup->setOpen(true);

    OFX::StringParamDescriptor *diagVersion = desc.defineStringParam(kParamDiagVersion);
    diagVersion->setLabels("Plugin Version", "Version", "Plugin Version");
    diagVersion->setEnabled(false);
    diagVersion->setParent(*diagGroup);

    OFX::StringParamDescriptor *diagStatus = desc.defineStringParam(kParamDiagStatus);
    diagStatus->setLabels("License Status", "Status", "License Status");
    diagStatus->setEnabled(false);
    diagStatus->setParent(*diagGroup);

    OFX::StringParamDescriptor *diagEdition = desc.defineStringParam(kParamDiagEdition);
    diagEdition->setLabels("Active Edition", "Edition", "Active Edition");
    diagEdition->setEnabled(false);
    diagEdition->setParent(*diagGroup);

    OFX::StringParamDescriptor *diagDecision = desc.defineStringParam(kParamDiagRenderDecision);
    diagDecision->setLabels("Render Decision", "Decision", "Render Decision");
    diagDecision->setEnabled(false);
    diagDecision->setParent(*diagGroup);

    OFX::StringParamDescriptor *diagPath = desc.defineStringParam(kParamDiagReceiptsPath);
    diagPath->setLabels("Receipts Path", "Path", "Receipts Path");
    diagPath->setEnabled(false);
    diagPath->setParent(*diagGroup);

    OFX::PushButtonParamDescriptor *btnRefresh = desc.definePushButtonParam(kParamBtnRefresh);
    btnRefresh->setLabels("Refresh License", "Refresh", "Refresh License");
    btnRefresh->setParent(*diagGroup);

    // 4. Updates & Notices parameter group
    OFX::GroupParamDescriptor *updGroup = desc.defineGroupParam(kGroupUpdates);
    updGroup->setLabels("NexKey Updates & Notices",
                        "Updates & Notices",
                        "NexKey Updates & Notices");
    updGroup->setOpen(true);

    OFX::StringParamDescriptor *updateStatus = desc.defineStringParam(kParamUpdateStatus);
    updateStatus->setLabels("Release Status", "Release", "Release Status");
    updateStatus->setEnabled(false);
    updateStatus->setParent(*updGroup);

    OFX::PushButtonParamDescriptor *btnCheck = desc.definePushButtonParam(kParamBtnCheckUpdates);
    btnCheck->setLabels("Check for Updates", "Check", "Check for Updates");
    btnCheck->setParent(*updGroup);

    OFX::StringParamDescriptor *noticeBanner = desc.defineStringParam(kParamNoticeBanner);
    noticeBanner->setLabels("Product Notice", "Notice", "Product Notice");
    noticeBanner->setEnabled(false);
    noticeBanner->setParent(*updGroup);

    OFX::PushButtonParamDescriptor *btnDismiss = desc.definePushButtonParam(kParamBtnDismissNotice);
    btnDismiss->setLabels("Dismiss Notice", "Dismiss", "Dismiss Notice");
    btnDismiss->setParent(*updGroup);

    OFX::PushButtonParamDescriptor *btnAction = desc.definePushButtonParam(kParamBtnOpenAction);
    btnAction->setLabels("Open Action Link", "Open Link", "Open Action Link");
    btnAction->setParent(*updGroup);
}

OFX::ImageEffect* CleanReferencePluginFactory::createInstance(OfxImageEffectHandle handle,
                                                              OFX::ContextEnum context)
{
    (void)context;
    return new CleanReferencePlugin(handle);
}

// ─── Entry Point Registration ────────────────────────────────────────────────

namespace OFX {
namespace Plugin {
void getPluginIDs(OFX::PluginFactoryArray &ids) {
    static CleanReferencePluginFactory p;
    ids.push_back(&p);
}
} // namespace Plugin
} // namespace OFX

