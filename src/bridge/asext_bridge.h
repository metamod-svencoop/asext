#pragma once

#include <GoldSrcExt/svencoop/server/angelscript_ext.h>

namespace asext_bridge
{

enum class BackendKind
{
    kUnavailable = 0,
    kHostNative,
    kLegacyInlineHook,
};

using RawCallHookFn     = void (*)(void* hook, int dummy, ...);
using RawCallCallableFn = int (*)(void* callable, int dummy, ...);

struct BridgeSession
{
    BackendKind                                    kind            = BackendKind::kUnavailable;
    goldsrcext::svencoop::ISvenCoopAngelScriptExt* host            = nullptr;
    RawCallHookFn                                  rawCallHook     = nullptr;
    RawCallCallableFn                              rawCallCallable = nullptr;
};

BridgeSession AcquireAngelScriptExt(void* serverHandle, void* serverBase) noexcept;
void          ReleaseAngelScriptExt(BridgeSession& session) noexcept;
void          OnServerActivatePost(BridgeSession& session) noexcept;
const char*   BackendKindName(BackendKind kind) noexcept;

#if !defined(_WIN64) && !defined(__x86_64__) && !defined(__LP64__)
BridgeSession AcquireLegacyAngelScriptExt(void* serverHandle, void* serverBase) noexcept;
void          ReleaseLegacyAngelScriptExt(BridgeSession& session) noexcept;
void          OnLegacyServerActivatePost(BridgeSession& session) noexcept;
#endif

} // namespace asext_bridge
