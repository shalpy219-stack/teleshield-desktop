#if defined(_WIN32)
  #if defined(TELESHIELD_CORE_EXPORTS)
    #define TELESHIELD_CORE_API __declspec(dllexport)
  #else
    #define TELESHIELD_CORE_API __declspec(dllimport)
  #endif
#else
  #define TELESHIELD_CORE_API
#endif

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Initializes the process-wide engine and precompiles all hot-path matchers.
 */
TELESHIELD_CORE_API int32_t teleshield_init(const char *config_dir_utf8);

/**
 * Returns 0=allow, 1=mask/blur, 2=block/drop. Invalid input fails closed with 2.
 */
TELESHIELD_CORE_API int32_t teleshield_inspect_text(const char *text_utf8);

/**
 * Returns 0=allow, 1=restrict, 2=block. Null username/title are treated as empty strings.
 */
TELESHIELD_CORE_API
int32_t teleshield_inspect_peer(int64_t peer_id,
                                const char *username_utf8,
                                const char *title_utf8);

TELESHIELD_CORE_API bool teleshield_is_search_locked(void);

/**
 * Returns 0 on success or 30 when the engine has not been initialized.
 */
TELESHIELD_CORE_API int32_t teleshield_set_search_lock(bool locked);

/**
 * Returns a process-lifetime, null-terminated UTF-8 version string.
 */
TELESHIELD_CORE_API const char *teleshield_version(void);

/**
 * Flushes in-memory audit counters and releases the engine. Safe to call more than once.
 */
TELESHIELD_CORE_API void teleshield_shutdown(void);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
