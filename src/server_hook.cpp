#include <extdll.h>

#include <dllapi.h>
#include <meta_api.h>

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "asext.h"
#include "bridge/asext_bridge.h"

namespace
{

using goldsrcext::HookHandle;
using goldsrcext::kInvalidHookHandle;
using goldsrcext::svencoop::ASDictIteratorStorage;
using goldsrcext::svencoop::ASFuncPtrDesc;
using goldsrcext::svencoop::ISvenCoopAngelScriptExt;

ISvenCoopAngelScriptExt* g_pHost = nullptr;

std::vector<HookHandle>                                           g_docHandles;
std::vector<HookHandle>                                           g_dirHandles;
std::vector<std::pair<fnScriptBuilderDefineCallback, HookHandle>> g_builderHandles;

void DocInitTrampoline(CASDocumentation* docs, void* userData) noexcept
{
    auto cb = reinterpret_cast<fnASDocInitCallback>(userData);
    if (cb)
        cb(docs);
}

void DirInitTrampoline(CASDirectoryList* dirList, void* userData) noexcept
{
    auto cb = reinterpret_cast<fnASDirInitCallback>(userData);
    if (cb)
        cb(dirList);
}

void ScriptBuilderDefineTrampoline(CScriptBuilder* builder, void* userData) noexcept
{
    auto cb = reinterpret_cast<fnScriptBuilderDefineCallback>(userData);
    if (cb)
        cb(builder);
}

void NativeCallHookTrampoline(void* hook, int argCount, ...)
{
    if (!g_pHost || !hook)
        return;

    std::va_list args;
    va_start(args, argCount);
    g_pHost->CallHookV(static_cast<CASHook*>(hook), argCount, args);
    va_end(args);
}

int NativeCallCallableTrampoline(void* callable, int argCount, ...)
{
    if (!g_pHost || !callable)
        return 0;

    std::va_list args;
    va_start(args, argCount);
    const bool ok = g_pHost->CallCallableV(static_cast<CASFunction*>(callable), argCount, args);
    va_end(args);
    return ok ? 1 : 0;
}

ASFuncPtrDesc MakeFuncPtrDesc(const void* funcPtr) noexcept
{
    ASFuncPtrDesc desc{};
    if (!funcPtr)
        return desc;

    const auto*       src     = static_cast<const asSFuncPtr*>(funcPtr);
    const std::size_t copyLen = sizeof(src->ptr.dummy) < ASFuncPtrDesc::kMaxBytes ? sizeof(src->ptr.dummy) : ASFuncPtrDesc::kMaxBytes;
    std::memcpy(desc.bytes, src->ptr.dummy, copyLen);
    desc.size = static_cast<std::uint8_t>(copyLen);
    desc.flag = src->flag;
    return desc;
}

} // namespace

fnASEXT_CallHook ASEXT_CallHook = NULL;

fnASEXT_CallCASBaseCallable ASEXT_CallCASBaseCallable = NULL;

void ASEXT_AttachHost(const asext_bridge::BridgeSession& session) noexcept
{
    g_pHost = session.host;
    g_docHandles.clear();
    g_dirHandles.clear();
    g_builderHandles.clear();

    if (session.kind == asext_bridge::BackendKind::kHostNative)
    {
        ASEXT_CallHook            = &NativeCallHookTrampoline;
        ASEXT_CallCASBaseCallable = &NativeCallCallableTrampoline;
    }
    else if (session.kind == asext_bridge::BackendKind::kLegacyInlineHook)
    {
        ASEXT_CallHook            = session.rawCallHook;
        ASEXT_CallCASBaseCallable = session.rawCallCallable;
    }
    else
    {
        ASEXT_CallHook            = NULL;
        ASEXT_CallCASBaseCallable = NULL;
    }
}

void ASEXT_DetachHost() noexcept
{
    if (g_pHost)
    {
        for (HookHandle h : g_docHandles)
            g_pHost->UnregisterDocInitCallback(h);
        for (HookHandle h : g_dirHandles)
            g_pHost->UnregisterDirInitCallback(h);
        for (const auto& entry : g_builderHandles)
            g_pHost->UnregisterScriptBuilderDefineCallback(entry.second);
    }

    g_docHandles.clear();
    g_dirHandles.clear();
    g_builderHandles.clear();
    g_pHost                   = nullptr;
    ASEXT_CallHook            = NULL;
    ASEXT_CallCASBaseCallable = NULL;
}

C_DLLEXPORT void ASEXT_RegisterObjectMethod(CASDocumentation* pthis, const char* docs, const char* name, const char* func, void* pfn, int type)
{
    if (g_pHost)
        g_pHost->RegisterObjectMethod(pthis, docs, name, func, pfn, static_cast<std::uint32_t>(type));
}

C_DLLEXPORT void ASEXT_RegisterObjectMethodEx(CASDocumentation* pthis, const char* docs, const char* name, const char* func, void* funcptr, int type)
{
    if (!g_pHost || !funcptr)
        return;
    ASFuncPtrDesc desc = MakeFuncPtrDesc(funcptr);
    g_pHost->RegisterObjectMethodEx(pthis, docs, name, func, &desc, static_cast<std::uint32_t>(type));
}

C_DLLEXPORT void ASEXT_RegisterObjectBehaviour(CASDocumentation* pthis, const char* docs, const char* name, int behaviour, const char* func, void* pfn, int type)
{
    if (g_pHost)
        g_pHost->RegisterObjectBehaviour(pthis, docs, name, behaviour, func, pfn, static_cast<std::uint32_t>(type));
}

C_DLLEXPORT void ASEXT_RegisterObjectBehaviourEx(CASDocumentation* pthis, const char* docs, const char* name, int behaviour, const char* func, void* funcptr, int type)
{
    if (!g_pHost || !funcptr)
        return;
    ASFuncPtrDesc desc = MakeFuncPtrDesc(funcptr);
    g_pHost->RegisterObjectBehaviourEx(pthis, docs, name, behaviour, func, &desc, static_cast<std::uint32_t>(type));
}

C_DLLEXPORT void ASEXT_RegisterObjectType(CASDocumentation* pthis, const char* docs, const char* name, int size, unsigned int flags)
{
    if (g_pHost)
        g_pHost->RegisterObjectType(pthis, docs, name, size, flags);
}

C_DLLEXPORT void ASEXT_RegisterObjectProperty(CASDocumentation* pthis, const char* docs, const char* name, const char* prop, int offset)
{
    if (g_pHost)
        g_pHost->RegisterObjectProperty(pthis, docs, name, prop, offset);
}

C_DLLEXPORT void ASEXT_RegisterGlobalProperty(CASDocumentation* pthis, const char* docs, const char* name, void* ptr)
{
    if (g_pHost)
        g_pHost->RegisterGlobalProperty(pthis, docs, name, ptr);
}

C_DLLEXPORT void ASEXT_RegisterGlobalFunction(CASDocumentation* pthis, const char* docs, const char* func, void* pfn, int type)
{
    if (g_pHost)
        g_pHost->RegisterGlobalFunction(pthis, docs, func, pfn, static_cast<std::uint32_t>(type));
}

C_DLLEXPORT void ASEXT_RegisterFuncDef(CASDocumentation* pthis, const char* docs, const char* funcdef)
{
    if (g_pHost)
        g_pHost->RegisterFuncDef(pthis, docs, funcdef);
}

C_DLLEXPORT void ASEXT_RegisterEnum(CASDocumentation* pthis, const char* docs, const char* enums, int enumtype)
{
    if (g_pHost)
        g_pHost->RegisterEnum(pthis, docs, enums, enumtype);
}

C_DLLEXPORT void ASEXT_RegisterEnumValue(CASDocumentation* pthis, const char* docs, const char* enums, const char* name, int value)
{
    if (g_pHost)
        g_pHost->RegisterEnumValue(pthis, docs, enums, name, value);
}

C_DLLEXPORT void ASEXT_SetDefaultNamespace(CASDocumentation* pthis, const char* ns)
{
    if (g_pHost)
        g_pHost->SetDefaultNamespace(pthis, ns);
}

C_DLLEXPORT asITypeInfo* ASEXT_CASBaseManager_GetTypeInfoByName(CASServerManager* /*pthis*/, const std_string* name)
{
    if (!g_pHost || !name)
        return NULL;
    return g_pHost->GetTypeInfoByName(const_cast<std_string*>(name)->c_str());
}

C_DLLEXPORT void ASEXT_CreateDirectory(CASDirectoryList* pthis, const char* path, unsigned char flags, unsigned char access_control, unsigned char permanent, unsigned char unk)
{
    if (g_pHost)
        g_pHost->CreateVirtualDirectory(pthis, path, flags, access_control, permanent, unk);
}

C_DLLEXPORT bool ASEXT_RegisterDocInitCallback(fnASDocInitCallback callback)
{
    if (!g_pHost || !callback)
        return false;
    HookHandle h = g_pHost->RegisterDocInitCallback(&DocInitTrampoline, reinterpret_cast<void*>(callback));
    if (h == kInvalidHookHandle)
        return false;
    g_docHandles.push_back(h);
    return true;
}

C_DLLEXPORT bool ASEXT_RegisterDirInitCallback(fnASDirInitCallback callback)
{
    if (!g_pHost || !callback)
        return false;
    HookHandle h = g_pHost->RegisterDirInitCallback(&DirInitTrampoline, reinterpret_cast<void*>(callback));
    if (h == kInvalidHookHandle)
        return false;
    g_dirHandles.push_back(h);
    return true;
}

C_DLLEXPORT bool ASEXT_RegisterScriptBuilderDefineCallback(fnScriptBuilderDefineCallback callback)
{
    if (!g_pHost || !callback)
        return false;
    HookHandle h = g_pHost->RegisterScriptBuilderDefineCallback(&ScriptBuilderDefineTrampoline, reinterpret_cast<void*>(callback));
    if (h == kInvalidHookHandle)
        return false;
    g_builderHandles.emplace_back(callback, h);
    return true;
}

C_DLLEXPORT bool ASEXT_UnregisterScriptBuilderDefineCallback(fnScriptBuilderDefineCallback callback)
{
    if (!g_pHost || !callback)
        return false;

    bool removed = false;
    auto it      = std::remove_if(g_builderHandles.begin(), g_builderHandles.end(), [&](const auto& entry) {
        if (entry.first == callback)
        {
            g_pHost->UnregisterScriptBuilderDefineCallback(entry.second);
            removed = true;
            return true;
        }
        return false;
    });
    g_builderHandles.erase(it, g_builderHandles.end());
    return removed;
}

C_DLLEXPORT void* ASEXT_RegisterHook(const char* docs, int stopMode, int type, int flags, const char* domain, const char* func, const char* args)
{
    if (!g_pHost)
        return NULL;
    return g_pHost->RegisterHook(
        docs,
        stopMode,
        static_cast<std::uint8_t>(type),
        static_cast<std::uint8_t>(flags),
        domain,
        func,
        args);
}

C_DLLEXPORT void ASEXT_CStringAssign(CString* pthis, const char* src, size_t len)
{
    if (g_pHost)
        g_pHost->CStringAssign(pthis, src, len);
}

C_DLLEXPORT void ASEXT_CString_Assign(CString* pthis, const char* src, size_t len)
{
    if (g_pHost)
        g_pHost->CStringAssign(pthis, src, len);
}

C_DLLEXPORT void ASEXT_CStringdtor(CString* pthis)
{
    if (g_pHost)
        g_pHost->CStringDestroy(pthis);
}

C_DLLEXPORT void ASEXT_CString_dtor(CString* pthis)
{
    if (g_pHost)
        g_pHost->CStringDestroy(pthis);
}

C_DLLEXPORT CASFunction* ASEXT_CreateCASFunction(aslScriptFunction* aslfn, CASModule* asmodule, int unk)
{
    if (!g_pHost)
        return NULL;
    return g_pHost->CreateCASFunction(reinterpret_cast<asIScriptFunction*>(aslfn), asmodule, unk != 0);
}

C_DLLEXPORT void ASEXT_ReleaseCASFunction(CASFunction* callback)
{
    if (g_pHost)
        g_pHost->ReleaseCASFunction(callback);
}

C_DLLEXPORT CASServerManager* ASEXT_GetServerManager()
{
    return g_pHost ? g_pHost->GetServerManager() : NULL;
}

C_DLLEXPORT asIScriptEngine* ASEXT_GetScriptEngine()
{
    return g_pHost ? g_pHost->GetScriptEngine() : NULL;
}

C_DLLEXPORT CASModule* ASEXT_GetCurrentModule()
{
    return g_pHost ? g_pHost->GetCurrentModule() : NULL;
}

C_DLLEXPORT bool ASEXT_CASRefCountedBaseClass_InternalRelease(void* ref)
{
    return g_pHost ? g_pHost->RefCountedInternalRelease(ref) : false;
}

C_DLLEXPORT void ASEXT_CScriptAny_Release(void* anywhat)
{
    if (g_pHost)
        g_pHost->ScriptAnyRelease(static_cast<CScriptAny*>(anywhat));
}

C_DLLEXPORT void ASEXT_CScriptArray_Release(void* anywhat)
{
    if (g_pHost)
        g_pHost->ScriptArrayRelease(static_cast<CScriptArray*>(anywhat));
}

C_DLLEXPORT void* ASEXT_GetCurrentContext()
{
    return g_pHost ? g_pHost->GetCurrentContext() : NULL;
}

C_DLLEXPORT void ASEXT_CScriptBuilder_DefineWord(CScriptBuilder* pthis, const char* word)
{
    if (g_pHost)
        g_pHost->ScriptBuilderDefineWord(pthis, word);
}

C_DLLEXPORT CScriptDictionary* ASEXT_CScriptDictionary_Create(void* pScriptEngine)
{
    return g_pHost ? g_pHost->DictCreate(static_cast<asIScriptEngine*>(pScriptEngine)) : NULL;
}

C_DLLEXPORT void ASEXT_CScriptDictionary_AddRef(CScriptDictionary* pScriptDictionary)
{
    if (g_pHost)
        g_pHost->DictAddRef(pScriptDictionary);
}

C_DLLEXPORT void ASEXT_CScriptDictionary_Release(CScriptDictionary* pScriptDictionary)
{
    if (g_pHost)
        g_pHost->DictRelease(pScriptDictionary);
}

C_DLLEXPORT void ASEXT_CScriptDictionary_Set(CScriptDictionary* pScriptDictionary, const CString* key, const void* val, int asTypeId)
{
    if (g_pHost)
        g_pHost->DictSet(pScriptDictionary, key, val, asTypeId);
}

C_DLLEXPORT bool ASEXT_CScriptDictionary_Get(CScriptDictionary* pScriptDictionary, const CString* key, void* val, int asTypeId)
{
    return g_pHost ? g_pHost->DictGet(pScriptDictionary, key, val, asTypeId) : false;
}

C_DLLEXPORT bool ASEXT_CScriptDictionary_Exists(CScriptDictionary* pScriptDictionary, const CString* key)
{
    return g_pHost ? g_pHost->DictExists(pScriptDictionary, key) : false;
}

C_DLLEXPORT bool ASEXT_CScriptDictionary_IsEmpty(CScriptDictionary* pScriptDictionary)
{
    return g_pHost ? g_pHost->DictIsEmpty(pScriptDictionary) : true;
}

C_DLLEXPORT unsigned int ASEXT_CScriptDictionary_GetSize(CScriptDictionary* pScriptDictionary)
{
    return g_pHost ? g_pHost->DictGetSize(pScriptDictionary) : 0u;
}

C_DLLEXPORT void ASEXT_CScriptDictionary_Delete(CScriptDictionary* pScriptDictionary, const CString* key)
{
    if (g_pHost)
        g_pHost->DictDelete(pScriptDictionary, key);
}

C_DLLEXPORT void ASEXT_CScriptDictionary_DeleteAll(CScriptDictionary* pScriptDictionary)
{
    if (g_pHost)
        g_pHost->DictDeleteAll(pScriptDictionary);
}

C_DLLEXPORT void ASEXT_CScriptDictionary_begin(CScriptDictionary* pScriptDictionary, CScriptDictionary_CIterator* it)
{
    if (g_pHost)
        g_pHost->DictBegin(pScriptDictionary, reinterpret_cast<ASDictIteratorStorage*>(it));
}

C_DLLEXPORT void ASEXT_CScriptDictionary_end(CScriptDictionary* pScriptDictionary, CScriptDictionary_CIterator* it)
{
    if (g_pHost)
        g_pHost->DictEnd(pScriptDictionary, reinterpret_cast<ASDictIteratorStorage*>(it));
}

C_DLLEXPORT bool ASEXT_CScriptDictionary_CIterator_operator_NE(CScriptDictionary_CIterator* a1, CScriptDictionary_CIterator* a2)
{
    if (!g_pHost)
        return false;
    return g_pHost->DictIterNotEqual(
        reinterpret_cast<const ASDictIteratorStorage*>(a1),
        reinterpret_cast<const ASDictIteratorStorage*>(a2));
}

C_DLLEXPORT bool ASEXT_CScriptDictionary_CIterator_GetValue(CScriptDictionary_CIterator* it, void* data, int typeId)
{
    if (!g_pHost)
        return false;
    return g_pHost->DictIterGetValue(reinterpret_cast<const ASDictIteratorStorage*>(it), data, typeId);
}

C_DLLEXPORT const CString* ASEXT_CScriptDictionary_CIterator_GetKey(CScriptDictionary_CIterator* it)
{
    if (!g_pHost)
        return NULL;
    return g_pHost->DictIterGetKey(reinterpret_cast<const ASDictIteratorStorage*>(it));
}

C_DLLEXPORT void ASEXT_CScriptDictionary_CIterator_operator_PP(CScriptDictionary_CIterator* it)
{
    if (g_pHost)
        g_pHost->DictIterIncrement(reinterpret_cast<ASDictIteratorStorage*>(it));
}
