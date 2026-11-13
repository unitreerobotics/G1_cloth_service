#pragma once

/**
 * Shared-library export/import macros for Windows DLL builds.
 * On Linux / macOS symbols are visible by default.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(JQ_RECEIVER_SHARED)
    #if defined(JQ_RECEIVER_BUILDING)
      #define JQ_RECEIVER_API __declspec(dllexport)
    #else
      #define JQ_RECEIVER_API __declspec(dllimport)
    #endif
  #else
    #define JQ_RECEIVER_API
  #endif
#else
  #define JQ_RECEIVER_API
#endif
