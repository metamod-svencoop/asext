#include "bridge/asext_bridge.h"

#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <angelscript.h>

#include <algorithm>
#include <cstring>
#include <vector>

#include "std_string.h"
#include "serverdef.h"

// Legacy 32-bit gamedata + inline hook symbols for original Sven Co-op server.dll / server.so.
PRIVATE_FUNCTION_DEFINE(CASHook_Call);
PRIVATE_FUNCTION_DEFINE(CASHook_CASHook);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterObjectType);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterObjectProperty);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterGlobalProperty);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterGlobalFunction);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterObjectMethod);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterObjectBehaviour);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterFuncDef);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterEnum);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_RegisterEnumValue);
PRIVATE_FUNCTION_DEFINE(CASDocumentation_SetDefaultNamespace);
PRIVATE_FUNCTION_DEFINE(CASBaseManager_GetTypeInfoByName);
PRIVATE_FUNCTION_DEFINE(CASDirectoryList_CreateDirectory);
PRIVATE_FUNCTION_DEFINE(CASFunction_Create);
PRIVATE_FUNCTION_DEFINE(CASBaseCallable_Call);
PRIVATE_FUNCTION_DEFINE(CASRefCountedBaseClass_InternalRelease);
PRIVATE_FUNCTION_DEFINE(CScriptAny_Release);
PRIVATE_FUNCTION_DEFINE(CScriptArray_Release);
PRIVATE_FUNCTION_DEFINE(CString_Assign);
PRIVATE_FUNCTION_DEFINE(CString_dtor);
PRIVATE_FUNCTION_DEFINE(asGetActiveContext);
PRIVATE_FUNCTION_DEFINE(CScriptBuilder_DefineWord);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_Create);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_AddRef);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_Release);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_Set);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_Get);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_Exists);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_IsEmpty);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_GetSize);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_Delete);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_DeleteAll);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_begin);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_end);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_CIterator_GetKey);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_CIterator_operator_NE);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_CIterator_GetValue);
PRIVATE_FUNCTION_DEFINE(CScriptDictionary_CIterator_operator_PP);

CASServerManager** g_pServerManager = NULL;

namespace
{

using goldsrcext::HookHandle;
using goldsrcext::kInvalidHookHandle;
using goldsrcext::svencoop::ASDictIteratorStorage;
using goldsrcext::svencoop::ASDirInitCallbackFn;
using goldsrcext::svencoop::ASDocInitCallbackFn;
using goldsrcext::svencoop::ASFuncPtrDesc;
using goldsrcext::svencoop::ASScriptBuilderDefineCallbackFn;
using goldsrcext::svencoop::ISvenCoopAngelScriptExt;

hook_t* g_phook_CASDocumentation_RegisterObjectType = NULL;
hook_t* g_phook_CASDirectoryList_CreateDirectory    = NULL;
hook_t* g_phook_CScriptBuilder_DefineWord           = NULL;

template <typename CallbackFn>
struct CallbackEntry
{
    HookHandle    handle;
    std::int32_t  altitude;
    std::uint64_t order;
    CallbackFn    callback;
    void*         userData;
};

template <typename CallbackFn>
HookHandle AddCallback(std::vector<CallbackEntry<CallbackFn>>& list,
                       HookHandle&                             nextHandle,
                       std::uint64_t&                          nextOrder,
                       CallbackFn                              callback,
                       void*                                   userData,
                       std::int32_t                            altitude) noexcept
{
    if (!callback)
        return kInvalidHookHandle;

    CallbackEntry<CallbackFn> entry{++nextHandle, altitude, ++nextOrder, callback, userData};
    auto                      pos = std::find_if(list.begin(), list.end(), [&](const CallbackEntry<CallbackFn>& cur) {
        if (entry.altitude != cur.altitude)
            return entry.altitude > cur.altitude;
        return entry.order < cur.order;
    });
    list.insert(pos, entry);
    return entry.handle;
}

template <typename CallbackFn>
bool RemoveCallback(std::vector<CallbackEntry<CallbackFn>>& list, HookHandle handle) noexcept
{
    if (handle == kInvalidHookHandle)
        return false;
    auto it = std::remove_if(list.begin(), list.end(), [handle](const CallbackEntry<CallbackFn>& e) {
        return e.handle == handle;
    });
    if (it == list.end())
        return false;
    list.erase(it, list.end());
    return true;
}

class CLegacySvenCoopAngelScriptExt final : public ISvenCoopAngelScriptExt
{
public:
    void Reset() noexcept
    {
        m_docCallbacks.clear();
        m_dirCallbacks.clear();
        m_builderCallbacks.clear();
        m_docInitialized = false;
        m_dirInitialized = false;
    }

    void FireDocInit(CASDocumentation* docs) noexcept
    {
        auto snapshot = m_docCallbacks;
        m_docCallbacks.clear();
        m_docInitialized = true;
        for (const auto& entry : snapshot)
            entry.callback(docs, entry.userData);
    }

    void FireDirInit(CASDirectoryList* dirList) noexcept
    {
        auto snapshot = m_dirCallbacks;
        m_dirCallbacks.clear();
        m_dirInitialized = true;
        for (const auto& entry : snapshot)
            entry.callback(dirList, entry.userData);
    }

    void FireScriptBuilderDefine(CScriptBuilder* builder) noexcept
    {
        auto snapshot = m_builderCallbacks;
        for (const auto& entry : snapshot)
            entry.callback(builder, entry.userData);
    }

    HookHandle RegisterDocInitCallback(ASDocInitCallbackFn cb, void* ud, std::int32_t altitude) noexcept override
    {
        if (m_docInitialized)
            return kInvalidHookHandle;
        return AddCallback(m_docCallbacks, m_nextHandle, m_nextOrder, cb, ud, altitude);
    }

    bool UnregisterDocInitCallback(HookHandle handle) noexcept override
    {
        return RemoveCallback(m_docCallbacks, handle);
    }

    bool IsDocInitialized() const noexcept override
    {
        return m_docInitialized;
    }

    HookHandle RegisterDirInitCallback(ASDirInitCallbackFn cb, void* ud, std::int32_t altitude) noexcept override
    {
        if (m_dirInitialized)
            return kInvalidHookHandle;
        return AddCallback(m_dirCallbacks, m_nextHandle, m_nextOrder, cb, ud, altitude);
    }

    bool UnregisterDirInitCallback(HookHandle handle) noexcept override
    {
        return RemoveCallback(m_dirCallbacks, handle);
    }

    bool IsDirInitialized() const noexcept override
    {
        return m_dirInitialized;
    }

    HookHandle RegisterScriptBuilderDefineCallback(ASScriptBuilderDefineCallbackFn cb, void* ud, std::int32_t altitude) noexcept override
    {
        return AddCallback(m_builderCallbacks, m_nextHandle, m_nextOrder, cb, ud, altitude);
    }

    bool UnregisterScriptBuilderDefineCallback(HookHandle handle) noexcept override
    {
        return RemoveCallback(m_builderCallbacks, handle);
    }

    void RegisterObjectType(CASDocumentation* docs, const char* docText, const char* name, int byteSize, std::uint32_t flags) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_RegisterObjectType(docs, SC_SERVER_PASS_DUMMYARG docText, name, byteSize, flags);
    }

    void RegisterObjectProperty(CASDocumentation* docs, const char* docText, const char* objectName, const char* propDecl, int offset) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_RegisterObjectProperty(docs, SC_SERVER_PASS_DUMMYARG docText, objectName, propDecl, offset);
    }

    void RegisterGlobalProperty(CASDocumentation* docs, const char* docText, const char* decl, void* address) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_RegisterGlobalProperty(docs, SC_SERVER_PASS_DUMMYARG docText, decl, address);
    }

    void RegisterGlobalFunction(CASDocumentation* docs, const char* docText, const char* decl, void* fnPtr, std::uint32_t callConv) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        asSFuncPtr reg;
        reg.CopyMethodPtr(&fnPtr, sizeof(fnPtr));
        reg.flag = (callConv == asCALL_GENERIC) ? 1 : 2;
        g_call_original_CASDocumentation_RegisterGlobalFunction(docs, SC_SERVER_PASS_DUMMYARG docText, decl, &reg, static_cast<int>(callConv), 0);
    }

    void RegisterObjectMethod(CASDocumentation* docs, const char* docText, const char* objectName, const char* decl, void* fnPtr, std::uint32_t callConv) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        asSFuncPtr reg;
        reg.CopyMethodPtr(&fnPtr, sizeof(fnPtr));
        switch (callConv)
        {
            case asCALL_GENERIC:
                reg.flag = 1;
                break;
            case asCALL_CDECL:
            case asCALL_CDECL_OBJFIRST:
            case asCALL_CDECL_OBJLAST:
                reg.flag = 2;
                break;
            default:
                reg.flag = 3;
                break;
        }
        g_call_original_CASDocumentation_RegisterObjectMethod(docs, SC_SERVER_PASS_DUMMYARG docText, objectName, decl, &reg, static_cast<int>(callConv));
    }

    void RegisterObjectMethodEx(CASDocumentation* docs, const char* docText, const char* objectName, const char* decl, const ASFuncPtrDesc* funcPtr, std::uint32_t callConv) noexcept override
    {
        if (!funcPtr)
            return;
        SC_SERVER_DUMMYVAR;
        asSFuncPtr        reg;
        const std::size_t maxCopy = sizeof(reg.ptr.dummy);
        const std::size_t copyLen = funcPtr->size < maxCopy ? funcPtr->size : maxCopy;
        if (copyLen > 0)
            reg.CopyMethodPtr(funcPtr->bytes, copyLen);
        reg.flag = funcPtr->flag;
        g_call_original_CASDocumentation_RegisterObjectMethod(docs, SC_SERVER_PASS_DUMMYARG docText, objectName, decl, &reg, static_cast<int>(callConv));
    }

    void RegisterObjectBehaviour(CASDocumentation* docs, const char* docText, const char* objectName, int behaviour, const char* decl, void* fnPtr, std::uint32_t callConv) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        asSFuncPtr reg;
        reg.CopyMethodPtr(&fnPtr, sizeof(fnPtr));
        reg.flag = (callConv == asCALL_GENERIC) ? 1 : 2;
        g_call_original_CASDocumentation_RegisterObjectBehaviour(docs, SC_SERVER_PASS_DUMMYARG docText, objectName, behaviour, decl, &reg, static_cast<int>(callConv), 0);
    }

    void RegisterObjectBehaviourEx(CASDocumentation* docs, const char* docText, const char* objectName, int behaviour, const char* decl, const ASFuncPtrDesc* funcPtr, std::uint32_t callConv) noexcept override
    {
        if (!funcPtr)
            return;
        SC_SERVER_DUMMYVAR;
        asSFuncPtr        reg;
        const std::size_t maxCopy = sizeof(reg.ptr.dummy);
        const std::size_t copyLen = funcPtr->size < maxCopy ? funcPtr->size : maxCopy;
        if (copyLen > 0)
            reg.CopyMethodPtr(funcPtr->bytes, copyLen);
        reg.flag = funcPtr->flag;
        g_call_original_CASDocumentation_RegisterObjectBehaviour(docs, SC_SERVER_PASS_DUMMYARG docText, objectName, behaviour, decl, &reg, static_cast<int>(callConv), 0);
    }

    void RegisterFuncDef(CASDocumentation* docs, const char* docText, const char* funcDef) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_RegisterFuncDef(docs, SC_SERVER_PASS_DUMMYARG docText, funcDef);
    }

    void RegisterEnum(CASDocumentation* docs, const char* docText, const char* enumName, int enumType) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_RegisterEnum(docs, SC_SERVER_PASS_DUMMYARG docText, enumName, enumType);
    }

    void RegisterEnumValue(CASDocumentation* docs, const char* docText, const char* enumName, const char* valueName, int value) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_RegisterEnumValue(docs, SC_SERVER_PASS_DUMMYARG docText, enumName, valueName, value);
    }

    void SetDefaultNamespace(CASDocumentation* docs, const char* ns) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDocumentation_SetDefaultNamespace(docs, SC_SERVER_PASS_DUMMYARG ns);
    }

    void CreateVirtualDirectory(CASDirectoryList* dirList, const char* path, std::uint8_t moduleFlags, std::uint8_t accessFlags, std::uint8_t dirFlags, std::uint8_t tempFlags) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_call_original_CASDirectoryList_CreateDirectory(dirList, SC_SERVER_PASS_DUMMYARG path, moduleFlags, accessFlags, dirFlags, tempFlags);
    }

    void ScriptBuilderDefineWord(CScriptBuilder* builder, const char* word) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        if (g_call_original_CScriptBuilder_DefineWord)
            g_call_original_CScriptBuilder_DefineWord(builder, SC_SERVER_PASS_DUMMYARG word);
        else
            g_pfn_CScriptBuilder_DefineWord(builder, SC_SERVER_PASS_DUMMYARG word);
    }

    CASServerManager* GetServerManager() noexcept override
    {
        return g_pServerManager ? *g_pServerManager : nullptr;
    }

    asIScriptEngine* GetScriptEngine() noexcept override
    {
        CASServerManager* mgr = GetServerManager();
        return mgr ? mgr->GetScriptEngine() : nullptr;
    }

    CASModule* GetCurrentModule() noexcept override
    {
        CASServerManager* mgr = GetServerManager();
        return mgr ? mgr->GetCurrentASModule() : nullptr;
    }

    asIScriptContext* GetCurrentContext() noexcept override
    {
        return static_cast<asIScriptContext*>(g_pfn_asGetActiveContext());
    }

    asITypeInfo* GetTypeInfoByName(const char* name) noexcept override
    {
        CASServerManager* mgr = GetServerManager();
        if (!mgr || !name)
            return nullptr;
        SC_SERVER_DUMMYVAR;
        std_string tmp;
        tmp.assign(name);
        return g_call_original_CASBaseManager_GetTypeInfoByName(mgr, SC_SERVER_PASS_DUMMYARG & tmp);
    }

    CASHook* RegisterHook(const char* docText, int stopMode, std::uint8_t dllTypes, std::uint8_t moduleTypes, const char* nameSpace, const char* funcName, const char* argList) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        CASHookRegistration reg;
        reg.unk      = 0;
        reg.stopMode = stopMode;
        reg.docs     = docText;

        CASHook* hook = new CASHook;
        m_hooks.push_back(hook);
        g_pfn_CASHook_CASHook(hook, SC_SERVER_PASS_DUMMYARG dllTypes, moduleTypes, nameSpace, funcName, argList, &reg);
        return hook;
    }

    bool CallHookV(CASHook*, int, std::va_list) noexcept override
    {
        // Legacy bridge uses raw variadic CASHook::Call directly via BridgeSession::rawCallHook.
        return false;
    }

    CASFunction* CreateCASFunction(asIScriptFunction* scriptFunc, CASModule* module, bool releaseFunc) noexcept override
    {
        return g_pfn_CASFunction_Create(reinterpret_cast<aslScriptFunction*>(scriptFunc), module, releaseFunc);
    }

    bool CallCallableV(CASFunction*, int, std::va_list) noexcept override
    {
        // Legacy bridge uses raw variadic CASBaseCallable::Call directly via BridgeSession::rawCallCallable.
        return false;
    }

    void ReleaseCASFunction(CASFunction* callable) noexcept override
    {
        if (!callable)
            return;
        if (g_pfn_CASRefCountedBaseClass_InternalRelease(callable->getReference()))
        {
#ifdef _WIN32
            callable->Release(1);
#else
            callable->Release();
#endif
        }
    }

    bool RefCountedInternalRelease(void* refPtr) noexcept override
    {
        return g_pfn_CASRefCountedBaseClass_InternalRelease(refPtr);
    }

    void CStringAssign(CString* str, const char* src, std::size_t len) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CString_Assign(str, SC_SERVER_PASS_DUMMYARG src, len);
    }

    void CStringDestroy(CString* str) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CString_dtor(str SC_SERVER_PASS_DUMMYARG2);
    }

    const char* CStringData(const CString* str) noexcept override
    {
        return str ? const_cast<CString*>(str)->c_str() : "";
    }

    std::size_t CStringLength(const CString* str) noexcept override
    {
        return str ? str->size() : 0;
    }

    void ScriptAnyRelease(CScriptAny* anyObj) noexcept override
    {
        g_pfn_CScriptAny_Release(anyObj);
    }

    void ScriptArrayRelease(CScriptArray* arrObj) noexcept override
    {
        g_pfn_CScriptArray_Release(arrObj);
    }

    bool ScriptArrayGetBuffer(CScriptArray* arrObj, void** outData, std::size_t* outSize) noexcept override
    {
        if (!arrObj)
            return false;
        if (outData)
            *outData = arrObj->data();
        if (outSize)
            *outSize = arrObj->size();
        return true;
    }

    bool BlobGetData(const CASBLOB* blob, const void** outData, std::size_t* outSize) noexcept override
    {
        if (!blob)
            return false;
        if (outData)
            *outData = const_cast<CASBLOB*>(blob)->data();
        if (outSize)
            *outSize = static_cast<std::size_t>(blob->m_size);
        return true;
    }

    CScriptDictionary* DictCreate(asIScriptEngine* engine) noexcept override
    {
        return g_pfn_CScriptDictionary_Create(engine);
    }

    void DictAddRef(CScriptDictionary* dict) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CScriptDictionary_AddRef(dict SC_SERVER_PASS_DUMMYARG2);
    }

    void DictRelease(CScriptDictionary* dict) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CScriptDictionary_Release(dict SC_SERVER_PASS_DUMMYARG2);
    }

    void DictSet(CScriptDictionary* dict, const CString* key, const void* val, int typeId) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CScriptDictionary_Set(dict, SC_SERVER_PASS_DUMMYARG key, val, typeId);
    }

    bool DictGet(CScriptDictionary* dict, const CString* key, void* val, int typeId) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_Get(dict, SC_SERVER_PASS_DUMMYARG key, val, typeId);
    }

    bool DictExists(CScriptDictionary* dict, const CString* key) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_Exists(dict, SC_SERVER_PASS_DUMMYARG key);
    }

    bool DictIsEmpty(CScriptDictionary* dict) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_IsEmpty(dict SC_SERVER_PASS_DUMMYARG2);
    }

    std::uint32_t DictGetSize(CScriptDictionary* dict) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_GetSize(dict SC_SERVER_PASS_DUMMYARG2);
    }

    bool DictDelete(CScriptDictionary* dict, const CString* key) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CScriptDictionary_Delete(dict, SC_SERVER_PASS_DUMMYARG key);
        return true;
    }

    void DictDeleteAll(CScriptDictionary* dict) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CScriptDictionary_DeleteAll(dict SC_SERVER_PASS_DUMMYARG2);
    }

    void DictBegin(CScriptDictionary* dict, ASDictIteratorStorage* outIter) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        auto* it = reinterpret_cast<CScriptDictionary_CIterator*>(outIter);
#ifdef _WIN32
        g_pfn_CScriptDictionary_begin(dict, SC_SERVER_PASS_DUMMYARG it);
#else
        g_pfn_CScriptDictionary_begin(it, dict);
#endif
    }

    void DictEnd(CScriptDictionary* dict, ASDictIteratorStorage* outIter) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        auto* it = reinterpret_cast<CScriptDictionary_CIterator*>(outIter);
#ifdef _WIN32
        g_pfn_CScriptDictionary_end(dict, SC_SERVER_PASS_DUMMYARG it);
#else
        g_pfn_CScriptDictionary_end(it, dict);
#endif
    }

    bool DictIterNotEqual(const ASDictIteratorStorage* lhs, const ASDictIteratorStorage* rhs) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_CIterator_operator_NE(
            reinterpret_cast<CScriptDictionary_CIterator*>(const_cast<ASDictIteratorStorage*>(lhs)),
            SC_SERVER_PASS_DUMMYARG reinterpret_cast<CScriptDictionary_CIterator*>(const_cast<ASDictIteratorStorage*>(rhs)));
    }

    bool DictIterGetValue(const ASDictIteratorStorage* iter, void* outVal, int typeId) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_CIterator_GetValue(
            reinterpret_cast<CScriptDictionary_CIterator*>(const_cast<ASDictIteratorStorage*>(iter)),
            SC_SERVER_PASS_DUMMYARG outVal,
            typeId);
    }

    const CString* DictIterGetKey(const ASDictIteratorStorage* iter) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        return g_pfn_CScriptDictionary_CIterator_GetKey(
            reinterpret_cast<CScriptDictionary_CIterator*>(const_cast<ASDictIteratorStorage*>(iter)) SC_SERVER_PASS_DUMMYARG2);
    }

    void DictIterIncrement(ASDictIteratorStorage* iter) noexcept override
    {
        SC_SERVER_DUMMYVAR;
        g_pfn_CScriptDictionary_CIterator_operator_PP(
            reinterpret_cast<CScriptDictionary_CIterator*>(iter) SC_SERVER_PASS_DUMMYARG2);
    }

    // Map script lifecycle hook chains are only provided by native GoldSrcExt hosts.
    HookHandle RegisterMapInitHook(goldsrcext::HookCallbackFn<bool>, void*, std::int32_t) noexcept override
    {
        return kInvalidHookHandle;
    }

    bool UnregisterMapInitHook(HookHandle) noexcept override
    {
        return false;
    }

    HookHandle RegisterMapActivateHook(goldsrcext::HookCallbackFn<bool>, void*, std::int32_t) noexcept override
    {
        return kInvalidHookHandle;
    }

    bool UnregisterMapActivateHook(HookHandle) noexcept override
    {
        return false;
    }

    HookHandle RegisterMapStartHook(goldsrcext::HookCallbackFn<bool>, void*, std::int32_t) noexcept override
    {
        return kInvalidHookHandle;
    }

    bool UnregisterMapStartHook(HookHandle) noexcept override
    {
        return false;
    }

private:
    std::vector<CallbackEntry<ASDocInitCallbackFn>>             m_docCallbacks;
    std::vector<CallbackEntry<ASDirInitCallbackFn>>             m_dirCallbacks;
    std::vector<CallbackEntry<ASScriptBuilderDefineCallbackFn>> m_builderCallbacks;
    std::vector<CASHook*>                                       m_hooks;
    HookHandle                                                  m_nextHandle     = 0;
    std::uint64_t                                               m_nextOrder      = 0;
    bool                                                        m_docInitialized = false;
    bool                                                        m_dirInitialized = false;
};

CLegacySvenCoopAngelScriptExt g_legacyHost;

int ResolveLegacySymbols(void* serverBase)
{
    FILL_FROM_GAMEDATA(server, CASHook_CASHook);
    FILL_FROM_GAMEDATA(server, CASHook_Call);
    FILL_FROM_GAMEDATA(server, CString_Assign);
    FILL_FROM_GAMEDATA(server, CString_dtor);
    FILL_FROM_GAMEDATA(server, asGetActiveContext);

    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterObjectType);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterObjectProperty);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterGlobalProperty);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterGlobalFunction);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterObjectMethod);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterObjectBehaviour);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterFuncDef);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterEnum);
    FILL_FROM_GAMEDATA(server, CASDocumentation_RegisterEnumValue);
    FILL_FROM_GAMEDATA(server, CASDocumentation_SetDefaultNamespace);
    FILL_FROM_GAMEDATA(server, CASBaseManager_GetTypeInfoByName);
    FILL_FROM_GAMEDATA(server, CASDirectoryList_CreateDirectory);
    FILL_FROM_GAMEDATA(server, CASFunction_Create);
    FILL_FROM_GAMEDATA(server, CASBaseCallable_Call);
    FILL_FROM_GAMEDATA(server, CASRefCountedBaseClass_InternalRelease);
    FILL_FROM_GAMEDATA(server, CScriptAny_Release);
    FILL_FROM_GAMEDATA(server, CScriptArray_Release);

    FILL_FROM_GAMEDATA(server, CScriptBuilder_DefineWord);

    FILL_FROM_GAMEDATA(server, CScriptDictionary_Create);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_AddRef);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_Release);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_Set);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_Get);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_Exists);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_IsEmpty);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_GetSize);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_Delete);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_DeleteAll);

    FILL_FROM_GAMEDATA(server, CScriptDictionary_begin);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_end);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_CIterator_operator_NE);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_CIterator_GetValue);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_CIterator_GetKey);
    FILL_FROM_GAMEDATA(server, CScriptDictionary_CIterator_operator_PP);

    VAR_FROM_GAMEDATA(server, g_pServerManager);

    return TRUE;
}

} // namespace

int SC_SERVER_DECL NewCASDocumentation_RegisterObjectType(CASDocumentation* pthis, SC_SERVER_DUMMYARG const char* docs, const char* name, int size, unsigned int flags)
{
    if (name && docs && !strcmp(name, "CSurvivalMode") && !strcmp(docs, "Survival Mode handler") && flags == 0x40001u)
    {
        g_legacyHost.FireDocInit(pthis);
    }

    return g_call_original_CASDocumentation_RegisterObjectType(pthis, SC_SERVER_PASS_DUMMYARG docs, name, size, flags);
}

void SC_SERVER_DECL NewCASDirectoryList_CreateDirectory(CASDirectoryList* pthis, SC_SERVER_DUMMYARG const char* path, unsigned char flags, unsigned char access_control, unsigned char permanent, unsigned char unk)
{
    if (path && !strcmp(path, "scripts/plugins/store") && flags == 2 && access_control == (1 | 2) && permanent == 1)
    {
        g_legacyHost.FireDirInit(pthis);
    }

    g_call_original_CASDirectoryList_CreateDirectory(pthis, SC_SERVER_PASS_DUMMYARG path, flags, access_control, permanent, unk);
}

void SC_SERVER_DECL NewCScriptBuilder_DefineWord(CScriptBuilder* pthis, SC_SERVER_DUMMYARG const char* word)
{
    if (g_call_original_CScriptBuilder_DefineWord)
        g_call_original_CScriptBuilder_DefineWord(pthis, SC_SERVER_PASS_DUMMYARG word);
    else
        g_pfn_CScriptBuilder_DefineWord(pthis, SC_SERVER_PASS_DUMMYARG word);

    if (word && !strcmp(word, "SERVER"))
    {
        g_legacyHost.FireScriptBuilderDefine(pthis);
    }
}

namespace asext_bridge
{

BridgeSession AcquireLegacyAngelScriptExt(void* /*serverHandle*/, void* serverBase) noexcept
{
    BridgeSession session;
    if (!serverBase)
        return session;

    g_legacyHost.Reset();
    if (!ResolveLegacySymbols(serverBase))
        return session;

    INSTALL_INLINEHOOK(CASDocumentation_RegisterObjectType);
    INSTALL_INLINEHOOK(CASDirectoryList_CreateDirectory);
    INSTALL_INLINEHOOK(CScriptBuilder_DefineWord);

    session.kind            = BackendKind::kLegacyInlineHook;
    session.host            = &g_legacyHost;
    session.rawCallHook     = reinterpret_cast<RawCallHookFn>(g_call_original_CASHook_Call);
    session.rawCallCallable = reinterpret_cast<RawCallCallableFn>(g_call_original_CASBaseCallable_Call);
    return session;
}

void ReleaseLegacyAngelScriptExt(BridgeSession& session) noexcept
{
    UNINSTALL_HOOK(CASDocumentation_RegisterObjectType);
    UNINSTALL_HOOK(CASDirectoryList_CreateDirectory);
    UNINSTALL_HOOK(CScriptBuilder_DefineWord);
    g_legacyHost.Reset();
    session = BridgeSession{};
}

void OnLegacyServerActivatePost(BridgeSession& /*session*/) noexcept
{
    if (g_legacyHost.IsDocInitialized())
    {
        UNINSTALL_HOOK(CASDocumentation_RegisterObjectType);
    }
    if (g_legacyHost.IsDirInitialized())
    {
        UNINSTALL_HOOK(CASDirectoryList_CreateDirectory);
    }
}

} // namespace asext_bridge
