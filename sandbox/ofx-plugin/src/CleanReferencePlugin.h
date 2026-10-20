// =============================================================================
// NexKeyRuntime Sandbox: Clean Reference OFX Plugin (Profile A Showcase)
// -----------------------------------------------------------------------------
// Integrated OpenFX Showcase:
//   1. Profile A offline licensing with < 1 ns atomic render decision.
//   2. Demo watermark rendering on CPU.
//   3. Live host inspector diagnostics (status, edition, receipts path).
//   4. Release updates check and vendor product notices directly in Inspector.
// =============================================================================

#pragma once

#include "ofxsImageEffect.h"
#include "ofxsProcessing.h"
#include "MCLicense.h"
#include "MCNotice.h"

#define kPluginIdentifier "app.mcnexus.nexkey.sandbox-ofx"
#define kPluginVersion "1.0.0"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

// Effect parameters
#define kParamExposure "exposure"
#define kParamExposureLabel "Exposure (Stops)"
#define kParamInvert "invert"
#define kParamInvertLabel "Invert Colors"

// License diagnostics group
#define kGroupDiagnostics "diagnosticsGroup"
#define kParamDiagVersion "diagVersion"
#define kParamDiagStatus "diagStatus"
#define kParamDiagEdition "diagEdition"
#define kParamDiagRenderDecision "diagRenderDecision"
#define kParamDiagReceiptsPath "diagReceiptsPath"
#define kParamBtnRefresh "btnRefresh"

// Updates & notices group
#define kGroupUpdates "updatesGroup"
#define kParamUpdateStatus "updateStatus"
#define kParamBtnCheckUpdates "btnCheckUpdates"
#define kParamNoticeBanner "noticeBanner"
#define kParamBtnDismissNotice "btnDismissNotice"
#define kParamBtnOpenAction "btnOpenAction"

class CleanReferencePluginFactory
    : public OFX::PluginFactoryHelper<CleanReferencePluginFactory>
{
public:
    CleanReferencePluginFactory();
    virtual void load() override {}
    virtual void unload() override {}
    virtual void describe(OFX::ImageEffectDescriptor& desc) override;
    virtual void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context) override;
    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context) override;
};

