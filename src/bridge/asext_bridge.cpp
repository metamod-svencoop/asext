#include "bridge/asext_bridge.h"

#include <extdll.h>
#include <meta_api.h>
#include <interface.h>

namespace asext_bridge
{

const char* BackendKindName(BackendKind kind) noexcept
{
    switch (kind)
    {
        case BackendKind::kHostNative:
            return "host-native";
        case BackendKind::kLegacyInlineHook:
            return "legacy-inline-hook";
        case BackendKind::kUnavailable:
        default:
            return "unavailable";
    }
}

BridgeSession AcquireAngelScriptExt(void* serverHandle, void* serverBase) noexcept
{
    BridgeSession session;

    if (serverHandle && gpMetaUtilFuncs && gpMetaUtilFuncs->pfnGetProcAddress)
    {
        auto createInterface = reinterpret_cast<CreateInterfaceFn>(
            gpMetaUtilFuncs->pfnGetProcAddress(reinterpret_cast<DLHANDLE>(serverHandle), CREATEINTERFACE_PROCNAME));

        if (createInterface)
        {
            int             returnCode = 0;
            IBaseInterface* raw        = createInterface(
                goldsrcext::svencoop::kSvenCoopAngelScriptExtInterfaceName,
                &returnCode);

            if (raw)
            {
                session.kind = BackendKind::kHostNative;
                session.host = static_cast<goldsrcext::svencoop::ISvenCoopAngelScriptExt*>(raw);
                return session;
            }
        }
    }

#if !defined(_WIN64) && !defined(__x86_64__) && !defined(__LP64__)
    return AcquireLegacyAngelScriptExt(serverHandle, serverBase);
#else
    (void)serverBase;
    if (gpMetaUtilFuncs)
    {
        LOG_ERROR(PLID,
                  "Game DLL does not expose %s; legacy 32-bit gamedata bridge is unavailable on 64-bit builds.",
                  goldsrcext::svencoop::kSvenCoopAngelScriptExtInterfaceName);
    }
    return session;
#endif
}

void ReleaseAngelScriptExt(BridgeSession& session) noexcept
{
#if !defined(_WIN64) && !defined(__x86_64__) && !defined(__LP64__)
    if (session.kind == BackendKind::kLegacyInlineHook)
    {
        ReleaseLegacyAngelScriptExt(session);
        return;
    }
#endif
    session = BridgeSession{};
}

void OnServerActivatePost(BridgeSession& session) noexcept
{
#if !defined(_WIN64) && !defined(__x86_64__) && !defined(__LP64__)
    if (session.kind == BackendKind::kLegacyInlineHook)
    {
        OnLegacyServerActivatePost(session);
    }
#else
    (void)session;
#endif
}

} // namespace asext_bridge
