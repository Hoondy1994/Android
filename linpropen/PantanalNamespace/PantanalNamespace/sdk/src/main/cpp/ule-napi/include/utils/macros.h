// TODO: Add CopyRight

#ifndef ULE_NAPI_UTILS_MACROS_H
#define ULE_NAPI_UTILS_MACROS_H

#ifndef NAPI_EXPORT
#ifdef WINDOWS_PLATFORM
#define NAPI_EXPORT __declspec(dllexport)
#else
#define NAPI_EXPORT __attribute__((visibility("default")))
#endif
#endif

#define NAPI_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;           \
  TypeName& operator=(const TypeName&) = delete

#endif /* ULE_NAPI_UTILS_MACROS_H */
