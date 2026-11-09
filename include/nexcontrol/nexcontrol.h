#ifndef NEXCONTROL_H
#define NEXCONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations to allow standalone usage or usage with OFX headers
struct OfxHost;
typedef struct OfxHost OfxHost;
#ifndef OFX_IMAGE_EFFECT_H
struct OfxImageEffectStruct;
typedef struct OfxImageEffectStruct* OfxImageEffectHandle;
#endif

struct NexKeyRuntimeLicenseHandle;
typedef struct NexKeyRuntimeLicenseHandle NexKeyRuntimeLicenseHandle;

#define NEXCONTROL_MAX_PARAMS 256

typedef enum NexControlResult {
    NEXCONTROL_OK                     = 0,
    NEXCONTROL_ERROR_INVALID_ARGUMENT = 1,
    NEXCONTROL_ERROR_NOT_INITIALIZED  = 2,
    NEXCONTROL_ERROR_IPC_FAILURE      = 3
} NexControlResult;

typedef enum NexControlParamType {
    NEXCONTROL_PARAM_DOUBLE = 1,
    NEXCONTROL_PARAM_INT    = 2,
    NEXCONTROL_PARAM_BOOL   = 3,
    NEXCONTROL_PARAM_CHOICE = 4
} NexControlParamType;

#if defined(_WIN32)
#  if defined(NEXCONTROL_EXPORTS)
#    define NEXCONTROL_API __declspec(dllexport)
#  else
#    define NEXCONTROL_API
#  endif
#else
#  define NEXCONTROL_API __attribute__((visibility("default")))
#endif

NEXCONTROL_API NexControlResult nexcontrol_init(OfxHost* host, NexKeyRuntimeLicenseHandle* license_handle);
NEXCONTROL_API NexControlResult nexcontrol_shutdown(void);
NEXCONTROL_API NexControlResult nexcontrol_register_instance(OfxImageEffectHandle instance, uint32_t* out_instance_id);
NEXCONTROL_API NexControlResult nexcontrol_unregister_instance(uint32_t instance_id);
NEXCONTROL_API NexControlResult nexcontrol_set_focus(uint32_t instance_id, bool is_focused);
NEXCONTROL_API NexControlResult nexcontrol_define_param_priority(const char* script_name, uint16_t priority_slot);
NEXCONTROL_API NexControlResult nexcontrol_add_param(uint32_t instance_id, const char* script_name, uint8_t param_type, double min_val, double max_val, double display_min, double display_max, double default_val);

#ifdef __cplusplus
}
#endif

#endif // NEXCONTROL_H
