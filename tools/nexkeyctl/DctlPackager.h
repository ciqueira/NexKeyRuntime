#ifndef NEXDCTL_DCTL_PACKAGER_H
#define NEXDCTL_DCTL_PACKAGER_H

#include <string>
#include <vector>

namespace nexdctl {

struct AssetKeys {
    std::string format;
    std::string productId;
    std::string keyId;
    std::string cekHex;
    std::string sigKeySeedHex;
    std::string sigPublicKeyHex;
};

struct PackageOptions {
    std::string inputPath;
    std::string pluginName;
    std::string bundleName;
    std::string pluginGroup;
    std::string productId;
    std::string version;
    std::string tenantId;
    std::string variant;
    std::string nexkeydatPath;
    std::string keysPath;
    std::string outputDir;
    std::string runnerPath;
    std::string dumpMslPath;
    std::string dumpOpenclPath;
    bool sandbox = false;
    bool debug = false;
    bool allowUncompiledMetalSource = false;
    std::vector<std::string> includeDirs;
};

bool generateAssetKeys(const std::string& productId, const std::string& outputPath, std::string& errOut);

bool loadAssetKeys(const std::string& keysPath, AssetKeys& outKeys, std::string& errOut);

bool packageAsset(PackageOptions& options, std::string& errOut);

} // namespace nexdctl

#endif // NEXDCTL_DCTL_PACKAGER_H

