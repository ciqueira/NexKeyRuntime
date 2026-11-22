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

#ifndef NEXCONTROL_MAX_PARAMS
#define NEXCONTROL_MAX_PARAMS 256
#endif

typedef enum NexControlResult {
    NEXCONTROL_OK                     = 0,
    NEXCONTROL_ERROR_INVALID_ARGUMENT = 1,
    NEXCONTROL_ERROR_NOT_INITIALIZED  = 2,
    NEXCONTROL_ERROR_IPC_FAILURE      = 3
} NexControlResult;

#ifndef NEXCONTROL_PARAM_TYPE_DEFINED
#define NEXCONTROL_PARAM_TYPE_DEFINED
typedef enum NexControlParamType {
    NEXCONTROL_PARAM_DOUBLE = 1,
    NEXCONTROL_PARAM_INT    = 2,
    NEXCONTROL_PARAM_BOOL   = 3,
    NEXCONTROL_PARAM_CHOICE = 4
} NexControlParamType;
#endif

// Parameter layout and semantic flags (shared with nexcontrol_protocol.h)
#ifndef NEXCONTROL_PARAM_FLAG_RESERVED_0
#define NEXCONTROL_PARAM_FLAG_RESERVED_0      0x01
#endif
#ifndef NEXCONTROL_PARAM_FLAG_LOGARITHMIC
#define NEXCONTROL_PARAM_FLAG_LOGARITHMIC     0x02
#endif
#ifndef NEXCONTROL_PARAM_FLAG_BIPOLAR
#define NEXCONTROL_PARAM_FLAG_BIPOLAR         0x04
#endif
#ifndef NEXCONTROL_PARAM_FLAG_GROUP_DEDICATED
#define NEXCONTROL_PARAM_FLAG_GROUP_DEDICATED 0x08
#endif
#ifndef NEXCONTROL_PARAM_FLAG_PINNED_SUBPAGES
#define NEXCONTROL_PARAM_FLAG_PINNED_SUBPAGES 0x10
#endif

#if defined(_WIN32)
#  if defined(NEXCONTROL_EXPORTS)
#    define NEXCONTROL_API __declspec(dllexport)
#  else
#    define NEXCONTROL_API
#  endif
#else
#  define NEXCONTROL_API __attribute__((visibility("default")))
#endif

typedef uint32_t (*NexControlFocusQueryCallback)(void);

NEXCONTROL_API NexControlResult nexcontrol_init(OfxHost* host, NexKeyRuntimeLicenseHandle* license_handle);
NEXCONTROL_API NexControlResult nexcontrol_shutdown(void);
NEXCONTROL_API NexControlResult nexcontrol_register_instance(OfxImageEffectHandle instance, uint32_t* out_instance_id);
NEXCONTROL_API NexControlResult nexcontrol_unregister_instance(uint32_t instance_id);
NEXCONTROL_API NexControlResult nexcontrol_set_focus(uint32_t instance_id, bool is_focused);
NEXCONTROL_API NexControlResult nexcontrol_set_focus_query_override(NexControlFocusQueryCallback callback);
NEXCONTROL_API NexControlResult nexcontrol_define_param_priority(const char* script_name, uint16_t priority_slot);
NEXCONTROL_API NexControlResult nexcontrol_add_param(
    uint32_t instance_id,
    const char* script_name,
    uint8_t param_type,
    double min_val,
    double max_val,
    double display_min,
    double display_max,
    double default_val);
NEXCONTROL_API NexControlResult nexcontrol_add_param_ex(
    uint32_t instance_id,
    const char* script_name,
    uint8_t param_type,
    double min_val,
    double max_val,
    double display_min,
    double display_max,
    double default_val,
    const char* group_label_override,
    uint8_t param_flags);
NEXCONTROL_API NexControlResult nexcontrol_begin_batch(uint32_t instance_id);
NEXCONTROL_API NexControlResult nexcontrol_end_batch(uint32_t instance_id);
NEXCONTROL_API NexControlResult nexcontrol_notify_user_edit(uint32_t instance_id, const char* script_name);

#ifdef __cplusplus
}
#endif

#endif // NEXCONTROL_H
