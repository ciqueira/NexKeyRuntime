// =============================================================================
// NexKeyRuntime SDK: 00 Standalone License Validator
// -----------------------------------------------------------------------------
// Fast, host-independent terminal validator for Profile A licensing.
// Validates local receipts, reports edition and atomic render decision (< 1 ns).
// =============================================================================

#include <iostream>
#include <iomanip>
#include "MCLicense.h"

int main() {
    std::cout << "=======================================================\n";
    std::cout << "   NexKeyRuntime SDK: Standalone License Validator     \n";
    std::cout << "   Environment: MCNexus Sandbox (mcnexus-sandbox)      \n";
    std::cout << "=======================================================\n\n";

    // 1. Initialize local receipt verification using MCLicense helper
    mc::License license;
    license.start();

    // 2. Query instant render decision (< 1 ns atomic check, zero I/O)
    bool allowed = license.allowed();
    mc::ReceiptEdition edition = license.edition();
    int days = license.daysRemaining();
    mc::LicenseReport report = license.report();

    // 3. Display diagnostic output
    std::cout << "Render Decision : " << (allowed ? "\033[32mALLOW (Render allowed)\033[0m" : "\033[31mDENY (Render blocked)\033[0m") << "\n";
    
    std::cout << "Active Edition  : ";
    switch (edition) {
        case mc::ReceiptEdition::Full:
            std::cout << "\033[32mFULL (Commercial Unlocked: No Watermark)\033[0m\n";
            break;
        case mc::ReceiptEdition::Demo:
            std::cout << "\033[33mDEMO (Render allowed with Demo Watermark)\033[0m\n";
            break;
        case mc::ReceiptEdition::Trial:
            std::cout << "\033[36mTRIAL (" << days << " days remaining)\033[0m\n";
            break;
        case mc::ReceiptEdition::Beta:
            std::cout << "\033[35mBETA (Testing Preview)\033[0m\n";
            break;
        default:
            std::cout << "\033[31mUNKNOWN / NOT ACTIVATED\033[0m\n";
            break;
    }

    std::cout << "License Status  : " << report.status << "\n";
    std::cout << "Validity Window : " << report.validity << "\n";
    std::cout << "Activation Info : " << report.activation << "\n";
    std::cout << "Receipts Dir    : " << report.receiptsDir << "\n";
    std::cout << "Receipts Found  : " << report.receiptsFound << "\n\n";

    if (!allowed) {
        std::cout << ">>> Notice: No active license was found on this system.\n";
        std::cout << "    To activate the Sandbox Demo license, run:\n";
        std::cout << "    nexkeyctl --activate --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08\n\n";
    }

    license.shutdown();
    return allowed ? 0 : 1;
}

