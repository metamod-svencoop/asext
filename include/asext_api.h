#pragma once

#include <angelscript.h>
#include "std_string.h"

#ifndef ASEXT_PLUGIN_PATH
#    if defined(_WIN64)
#        define ASEXT_PLUGIN_PATH "addons/metamod/dlls/win64/asext.dll"
#    elif defined(_WIN32)
#        define ASEXT_PLUGIN_PATH "addons/metamod/dlls/win32/asext.dll"
#    elif defined(__x86_64__) || defined(__amd64__)
#        define ASEXT_PLUGIN_PATH "addons/metamod/dlls/linux64/asext.so"
#    else
#        define ASEXT_PLUGIN_PATH "addons/metamod/dlls/linux/asext.so"
#    endif
#endif

const int StopMode_ON_HANDLED     = 0;
const int StopMode_MODULE_HANDLED = 1;
const int StopMode_CALL_ALL       = 2;

const int ASFlag_MapScript = 1;
const int ASFlag_Plugin    = 2;

const int ASFileAccessControl_Read  = 1;
const int ASFileAccessControl_Write = 2;

class CASDocumentation
{
public:
};

class CASDirectoryList
{
public:
};

class CASModule
{
public:
};

class asContext
{
public:
    virtual void unk0()  = 0;
    virtual void unk1()  = 0;
    virtual void unk2()  = 0;
    virtual void unk3()  = 0;
    virtual void unk4()  = 0;
    virtual void unk5()  = 0;
    virtual void unk6()  = 0;
    virtual void unk7()  = 0;
    virtual void unk8()  = 0;
    virtual void unk9()  = 0;
    virtual void unk10() = 0;
    virtual void unk11() = 0;
    virtual void unk12() = 0;
    virtual void unk13() = 0;
    virtual void unk14() = 0;
    virtual void unk15() = 0;
    virtual void unk16() = 0;
    virtual void unk17() = 0;
    virtual void unk18() = 0;
    virtual void unk19() = 0;
    virtual void unk20() = 0;
    virtual void unk21() = 0;
    virtual void unk22() = 0;
    virtual void unk23() = 0;
    virtual int  unk24() = 0;
    virtual int  unk25() = 0;
    virtual int  unk26() = 0;
};

class aslScriptFunction
{
public:
};

class CASFunction
{
public:
#ifdef _WIN32
#    define CASFunction_dtor_firstarg 1
    virtual void Release(char a1) = 0;
#else
#    define CASFunction_dtor_firstarg
    virtual void Unknown() = 0;
    virtual void Release() = 0;
#endif

    void* getReference()
    {
        return &ref;
    }

    aslScriptFunction* getScriptFunction()
    {
        return aslfn;
    }

    CASModule* getModule()
    {
        return asmodule;
    }

    asContext* getContext()
    {
        return ascontext;
    }

    int                ref;
    aslScriptFunction* aslfn;
    CASModule*         asmodule;
    asContext*         ascontext;
};

#define ASEXT_DereferenceCASFunction(callback)                                             \
    do                                                                                     \
    {                                                                                      \
        if (ASEXT_ReleaseCASFunction)                                                      \
        {                                                                                  \
            ASEXT_ReleaseCASFunction(callback);                                            \
        }                                                                                  \
        else if (ASEXT_CASRefCountedBaseClass_InternalRelease((callback)->getReference())) \
        {                                                                                  \
            (callback)->Release(CASFunction_dtor_firstarg);                                \
        }                                                                                  \
    } while (0)

class asIScriptEngine;

class CASServerManager
{
public:
    asIScriptEngine* GetScriptEngine() const
    {
        return scriptEngine;
    }
    CASModule* GetCurrentASModule() const
    {
        return curModule;
    }

public:
    void*            vptr;         // 0  (32-bit: 0, 64-bit: 0)
    bool             unk_b1;       // 4  (32-bit: 4, 64-bit: 8)
    bool             unk_b2;       // 5  (32-bit: 5, 64-bit: 9)
    bool             unk_b3;       // 6  (32-bit: 6, 64-bit: 10)
    void*            m_pFactory;   // 8  (32-bit: 8, 64-bit: 16)
    asIScriptEngine* scriptEngine; // 12 (32-bit: 12, 64-bit: 24)
    void*            m_pDebugger;  // 16 (32-bit: 16, 64-bit: 32)
    void*            m_pMapModule; // 20 (32-bit: 20, 64-bit: 40)
    CASModule*       curModule;    // 24 (32-bit: 24, 64-bit: 48)
};

class CString;
class CScriptBuilder;
class CScriptDictionary;
class CScriptDictionary_CIterator;

typedef void (*fnASDocInitCallback)(CASDocumentation* pASDoc);
typedef void (*fnASDirInitCallback)(CASDirectoryList* pASDir);
typedef void (*fnScriptBuilderDefineCallback)(CScriptBuilder* pScriptBuilder);

/*
	Callbacks must be registered before AngelScript initialization
*/
typedef bool (*fnASEXT_RegisterDocInitCallback)(fnASDocInitCallback callback);

extern fnASEXT_RegisterDocInitCallback ASEXT_RegisterDocInitCallback;


/*
	Callbacks must be registered before AngelScript initialization
*/

typedef bool (*fnASEXT_RegisterDirInitCallback)(fnASDirInitCallback callback);

extern fnASEXT_RegisterDirInitCallback ASEXT_RegisterDirInitCallback;


/*
	Callbacks must be registered before module start
*/

typedef bool (*fnASEXT_RegisterScriptBuilderDefineCallback)(fnScriptBuilderDefineCallback callback);

extern fnASEXT_RegisterScriptBuilderDefineCallback ASEXT_RegisterScriptBuilderDefineCallback;

typedef bool (*fnASEXT_UnregisterScriptBuilderDefineCallback)(fnScriptBuilderDefineCallback callback);

extern fnASEXT_UnregisterScriptBuilderDefineCallback ASEXT_UnregisterScriptBuilderDefineCallback;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterGlobalFunction)(CASDocumentation* pASDoc, const char* docs, const char* func, void* pfn, int type);

extern fnASEXT_RegisterGlobalFunction ASEXT_RegisterGlobalFunction;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterObjectMethod)(CASDocumentation* pASDoc, const char* docs, const char* name, const char* func, void* pfn, int type);

extern fnASEXT_RegisterObjectMethod ASEXT_RegisterObjectMethod;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterObjectMethodEx)(CASDocumentation* pASDoc, const char* docs, const char* name, const char* func, void* funcPtr, int type);

extern fnASEXT_RegisterObjectMethodEx ASEXT_RegisterObjectMethodEx;

const int ObjectBehaviour_Constructor = 0;
const int ObjectBehaviour_Destructor  = 2;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterObjectBehaviour)(CASDocumentation* pASDoc, const char* docs, const char* name, int behaviour, const char* func, void* pfn, int type);

extern fnASEXT_RegisterObjectBehaviour ASEXT_RegisterObjectBehaviour;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterObjectBehaviourEx)(CASDocumentation* pASDoc, const char* docs, const char* name, int behaviour, const char* func, void* funcPtr, int type);

extern fnASEXT_RegisterObjectBehaviourEx ASEXT_RegisterObjectBehaviourEx;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterObjectType)(CASDocumentation* pASDoc, const char* docs, const char* name, int unk, unsigned int flags);

extern fnASEXT_RegisterObjectType ASEXT_RegisterObjectType;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterObjectProperty)(CASDocumentation* pASDoc, const char* docs, const char* name, const char* prop, int offset);

extern fnASEXT_RegisterObjectProperty ASEXT_RegisterObjectProperty;


/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterGlobalProperty)(CASDocumentation* pASDoc, const char* docs, const char* name, void* ptr);

extern fnASEXT_RegisterGlobalProperty ASEXT_RegisterGlobalProperty;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterFuncDef)(CASDocumentation* pASDoc, const char* docs, const char* funcdef);

extern fnASEXT_RegisterFuncDef ASEXT_RegisterFuncDef;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterEnum)(CASDocumentation* pASDoc, const char* docs, const char* enums, int enumtype);

extern fnASEXT_RegisterEnum ASEXT_RegisterEnum;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_RegisterEnumValue)(CASDocumentation* pASDoc, const char* docs, const char* enums, const char* name, int value);

extern fnASEXT_RegisterEnumValue ASEXT_RegisterEnumValue;

/*
	Must be called inside DocInitCallback
*/
typedef void (*fnASEXT_SetDefaultNamespace)(CASDocumentation* pASDoc, const char* ns);

extern fnASEXT_SetDefaultNamespace ASEXT_SetDefaultNamespace;

/*
	Can be used to get asITypeInfo* for "string" or whatever
*/
typedef asITypeInfo* (*fnASEXT_CASBaseManager_GetTypeInfoByName)(CASServerManager* pthis, const std_string* name);

extern fnASEXT_CASBaseManager_GetTypeInfoByName ASEXT_CASBaseManager_GetTypeInfoByName;

typedef void (*fnASEXT_CreateDirectory)(CASDirectoryList* pASDirList, const char* path, unsigned char flags, unsigned char access_control, unsigned char permanent, unsigned char unk);
extern fnASEXT_CreateDirectory ASEXT_CreateDirectory;

typedef void (*fnASEXT_CStringAssign)(CString* pthis, const char* src, size_t len);

extern fnASEXT_CStringAssign ASEXT_CStringAssign;

typedef void (*fnASEXT_CStringdtor)(CString* pthis);

extern fnASEXT_CStringdtor ASEXT_CStringdtor;

/*
	Must be registered before call hook
*/
typedef void* (*fnASEXT_RegisterHook)(const char* docs, int stopMode, int type, int flags, const char* domain, const char* func, const char* args);

extern fnASEXT_RegisterHook ASEXT_RegisterHook;

typedef void (*fnASEXT_CallHook)(void* hook, int unk, ...);

extern fnASEXT_CallHook* ASEXT_CallHook;

typedef int (*fnASEXT_CallCASBaseCallable)(void* callable, int dummy, ...);

extern fnASEXT_CallCASBaseCallable* ASEXT_CallCASBaseCallable;

/*
	It's possible to get current IScriptEngine with ASEXT_GetServerManager()->scriptEngine
*/

typedef CASServerManager* (*fnASEXT_GetServerManager)();

extern fnASEXT_GetServerManager ASEXT_GetServerManager;

typedef asIScriptEngine* (*fnASEXT_GetScriptEngine)();

extern fnASEXT_GetScriptEngine ASEXT_GetScriptEngine;

typedef CASModule* (*fnASEXT_GetCurrentModule)();

extern fnASEXT_GetCurrentModule ASEXT_GetCurrentModule;

typedef CASFunction* (*fnASEXT_CreateCASFunction)(aslScriptFunction* aslfn, CASModule* asmodule, int unk);

extern fnASEXT_CreateCASFunction ASEXT_CreateCASFunction;

typedef void (*fnASEXT_ReleaseCASFunction)(CASFunction* callback);

extern fnASEXT_ReleaseCASFunction ASEXT_ReleaseCASFunction;

typedef bool (*fnASEXT_CASRefCountedBaseClass_InternalRelease)(void* ref);

extern fnASEXT_CASRefCountedBaseClass_InternalRelease ASEXT_CASRefCountedBaseClass_InternalRelease;

typedef void (*fnASEXT_CScriptAny_Release)(void* anywhat);

typedef void (*fnASEXT_CScriptArray_Release)(void* anywhat);

extern fnASEXT_CScriptAny_Release ASEXT_CScriptAny_Release;

extern fnASEXT_CScriptArray_Release ASEXT_CScriptArray_Release;

typedef CScriptDictionary* (*fnASEXT_CScriptDictionary_Create)(void* pScriptEngine);

typedef void (*fnASEXT_CScriptDictionary_AddRef)(CScriptDictionary* pScriptDictionary);

typedef void (*fnASEXT_CScriptDictionary_Release)(CScriptDictionary* pScriptDictionary);

typedef void (*fnASEXT_CScriptDictionary_Set)(CScriptDictionary* pScriptDictionary, const CString* key, const void* val, int asTypeId); //asETypeIdFlags

typedef bool (*fnASEXT_CScriptDictionary_Get)(CScriptDictionary* pScriptDictionary, const CString* key, void* val, int asTypeId); //asETypeIdFlags

typedef bool (*fnASEXT_CScriptDictionary_Exists)(CScriptDictionary* pScriptDictionary, const CString* key);

typedef bool (*fnASEXT_CScriptDictionary_IsEmpty)(CScriptDictionary* pScriptDictionary);

typedef unsigned int (*fnASEXT_CScriptDictionary_GetSize)(CScriptDictionary* pScriptDictionary);

typedef void (*fnASEXT_CScriptDictionary_Delete)(CScriptDictionary* pScriptDictionary, const CString* key);

typedef void (*fnASEXT_CScriptDictionary_DeleteAll)(CScriptDictionary* pScriptDictionary);

extern fnASEXT_CScriptDictionary_Create ASEXT_CScriptDictionary_Create;

extern fnASEXT_CScriptDictionary_AddRef ASEXT_CScriptDictionary_AddRef;

extern fnASEXT_CScriptDictionary_Release ASEXT_CScriptDictionary_Release;

extern fnASEXT_CScriptDictionary_Set ASEXT_CScriptDictionary_Set;

extern fnASEXT_CScriptDictionary_Get ASEXT_CScriptDictionary_Get;

extern fnASEXT_CScriptDictionary_Exists ASEXT_CScriptDictionary_Exists;

extern fnASEXT_CScriptDictionary_IsEmpty ASEXT_CScriptDictionary_IsEmpty;

extern fnASEXT_CScriptDictionary_GetSize ASEXT_CScriptDictionary_GetSize;

extern fnASEXT_CScriptDictionary_Delete ASEXT_CScriptDictionary_Delete;

extern fnASEXT_CScriptDictionary_DeleteAll ASEXT_CScriptDictionary_DeleteAll;

typedef void (*fnASEXT_CScriptDictionary_begin)(CScriptDictionary* pDict, CScriptDictionary_CIterator* itor);

typedef void (*fnASEXT_CScriptDictionary_end)(CScriptDictionary* pDict, CScriptDictionary_CIterator* itor);

typedef bool (*fnASEXT_CScriptDictionary_CIterator_operator_NE)(CScriptDictionary_CIterator* a1, CScriptDictionary_CIterator* a2);

typedef bool (*fnASEXT_CScriptDictionary_CIterator_GetValue)(CScriptDictionary_CIterator* it, void* data, int typeId);

typedef const CString* (*fnASEXT_CScriptDictionary_CIterator_GetKey)(CScriptDictionary_CIterator* it);

typedef void (*fnASEXT_CScriptDictionary_CIterator_operator_PP)(CScriptDictionary_CIterator* it);

extern fnASEXT_CScriptDictionary_begin ASEXT_CScriptDictionary_begin;

extern fnASEXT_CScriptDictionary_end ASEXT_CScriptDictionary_end;

extern fnASEXT_CScriptDictionary_CIterator_operator_NE ASEXT_CScriptDictionary_CIterator_operator_NE;

extern fnASEXT_CScriptDictionary_CIterator_GetValue ASEXT_CScriptDictionary_CIterator_GetValue;

extern fnASEXT_CScriptDictionary_CIterator_GetKey ASEXT_CScriptDictionary_CIterator_GetKey;

extern fnASEXT_CScriptDictionary_CIterator_operator_PP ASEXT_CScriptDictionary_CIterator_operator_PP;

typedef void* (*fnASEXT_GetCurrentContext)();
extern fnASEXT_GetCurrentContext ASEXT_GetCurrentContext;

typedef void (*fnASEXT_CScriptBuilder_DefineWord)(CScriptBuilder* pthis, const char* word);
extern fnASEXT_CScriptBuilder_DefineWord ASEXT_CScriptBuilder_DefineWord;

#if defined(_WIN32) && !defined(_WIN64)

#    define SC_SERVER_DECL                  __fastcall
#    define SC_SERVER_DUMMYARG_NOCOMMA      , int dummy
#    define SC_SERVER_DUMMYARG              int dummy,
#    define SC_SERVER_PASS_DUMMYARG         dummy,
#    define SC_SERVER_PASS_DUMMYARG_NOCOMMA dummy
#    define SC_SERVER_CDECL                 __cdecl

#else

#    define SC_SERVER_DECL
#    define SC_SERVER_DUMMYARG_NOCOMMA
#    define SC_SERVER_DUMMYARG
#    define SC_SERVER_PASS_DUMMYARG
#    define SC_SERVER_PASS_DUMMYARG_NOCOMMA
#    define SC_SERVER_CDECL

#endif

class CString
{
public:
    const char* c_str() const
    {
        return m_ptr ? m_ptr : "";
    }
    size_t length() const
    {
        return m_len;
    }
    char chatAt(size_t offset) const
    {
        return offset >= m_len ? m_ptr[0] : m_ptr[offset];
    }
    bool empty() const
    {
        return m_len == 0;
    }
    size_t size() const
    {
        return m_len;
    }
    void assign(const char* src, size_t len)
    {
        ASEXT_CStringAssign(this, src, len);
    }
    void dtor()
    {
        ASEXT_CStringdtor(this);
    }

    char         m_buf[20];     // 0  (32-bit: 0..19, 64-bit: 0..19 + 4B pad)
    char*        m_ptr;         // 20 (32-bit), 24 (64-bit)
    unsigned int m_len;         // 24 (32-bit), 32 (64-bit)
    unsigned int m_capacity;    // 28 (32-bit), 36 (64-bit)
    unsigned int m_tokenizeIdx; // 32 (32-bit), 40 (64-bit)
};

class CASBLOB
{
public:
    void* data() const
    {
        return m_buf;
    }
    size_t size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    int          unk1;           // 0  (m_iRefCount; 4B pad on 64-bit)
    void*        m_buf;          // 4  (32-bit), 8  (64-bit)
    unsigned int m_size;         // 8  (32-bit), 16 (64-bit)
    unsigned int m_read_offset;  // 12 (32-bit), 20 (64-bit)
    unsigned int m_write_offset; // 16 (32-bit), 24 (64-bit)
    char         m_can_resize;   // 20 (32-bit), 28 (64-bit)
};

class CScriptArrayBuffer
{
public:
    void* data()
    {
        return m_buf;
    }
    size_t size()
    {
        return m_size;
    }
    unsigned int m_unk1;   // 0 (maxElements)
    unsigned int m_size;   // 4 (numElements)
    char         m_buf[1]; // 8
};

class CScriptArray
{
public:
    void* data()
    {
        return m_buffer->data();
    }

    size_t size()
    {
        return m_buffer->m_size;
    }

    void*               m_vptr;          // 0  (32-bit), 0  (64-bit)
    int                 m_refCount;      // 4  (32-bit), 8  (64-bit)
    bool                m_gcFlag;        // 8  (32-bit), 12 (64-bit)
    asITypeInfo*        m_objType;       // 12 (32-bit), 16 (64-bit)
    CScriptArrayBuffer* m_buffer;        // 16 (32-bit), 24 (64-bit)
    int                 m_elementSize;   // 20 (32-bit), 32 (64-bit)
    int                 m_ElementTypeId; // 24 (32-bit), 36 (64-bit)
};

class CScriptDictionary
{
public:
};

class CScriptDictionary_CIterator
{
public:
    void* cur;
    void* next;
};

#define IMPORT_ASEXT_API(asext)                                                  \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CallHook);                                \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CallCASBaseCallable);                     \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterDocInitCallback);                 \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterDirInitCallback);                 \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterScriptBuilderDefineCallback);     \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_UnregisterScriptBuilderDefineCallback);   \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterGlobalFunction);                  \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterObjectMethod);                    \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterObjectMethodEx);                  \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterObjectBehaviour);                 \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterObjectBehaviourEx);               \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterObjectType);                      \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterObjectProperty);                  \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterGlobalProperty);                  \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterHook);                            \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterFuncDef);                         \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterEnum);                            \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_RegisterEnumValue);                       \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_SetDefaultNamespace);                     \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CreateDirectory);                         \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CStringAssign);                           \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CStringdtor);                             \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_GetServerManager);                        \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_GetScriptEngine);                         \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_GetCurrentModule);                        \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CreateCASFunction);                       \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_ReleaseCASFunction);                      \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CASRefCountedBaseClass_InternalRelease);  \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptAny_Release);                      \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptArray_Release);                    \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_Create);                \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_AddRef);                \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_Release);               \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_Set);                   \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_Get);                   \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_Exists);                \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_IsEmpty);               \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_GetSize);               \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_Delete);                \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_DeleteAll);             \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_begin);                 \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_end);                   \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_CIterator_operator_NE); \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_CIterator_GetValue);    \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_CIterator_GetKey);      \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptDictionary_CIterator_operator_PP); \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_GetCurrentContext);                       \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CScriptBuilder_DefineWord);               \
    IMPORT_FUNCTION_DLSYM(asext, ASEXT_CASBaseManager_GetTypeInfoByName);


#define IMPORT_ASEXT_API_DEFINE()                                          \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterDocInitCallback);                 \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterDirInitCallback);                 \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterScriptBuilderDefineCallback);     \
    IMPORT_FUNCTION_DEFINE(ASEXT_UnregisterScriptBuilderDefineCallback);   \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterGlobalFunction);                  \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterObjectMethod);                    \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterObjectMethodEx);                  \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterObjectBehaviour);                 \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterObjectBehaviourEx);               \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterObjectType);                      \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterObjectProperty);                  \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterGlobalProperty);                  \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterFuncDef);                         \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterEnum);                            \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterEnumValue);                       \
    IMPORT_FUNCTION_DEFINE(ASEXT_SetDefaultNamespace);                     \
    IMPORT_FUNCTION_DEFINE(ASEXT_RegisterHook);                            \
    IMPORT_FUNCTION_DEFINE(ASEXT_CreateDirectory);                         \
    IMPORT_FUNCTION_DEFINE(ASEXT_CStringAssign);                           \
    IMPORT_FUNCTION_DEFINE(ASEXT_CStringdtor);                             \
    IMPORT_FUNCTION_DEFINE(ASEXT_GetServerManager);                        \
    IMPORT_FUNCTION_DEFINE(ASEXT_GetScriptEngine);                         \
    IMPORT_FUNCTION_DEFINE(ASEXT_GetCurrentModule);                        \
    IMPORT_FUNCTION_DEFINE(ASEXT_CreateCASFunction);                       \
    IMPORT_FUNCTION_DEFINE(ASEXT_ReleaseCASFunction);                      \
    IMPORT_FUNCTION_DEFINE(ASEXT_CASRefCountedBaseClass_InternalRelease);  \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptAny_Release);                      \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptArray_Release);                    \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_Create);                \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_AddRef);                \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_Release);               \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_Set);                   \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_Get);                   \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_Exists);                \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_IsEmpty);               \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_GetSize);               \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_Delete);                \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_DeleteAll);             \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_begin);                 \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_end);                   \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_CIterator_operator_NE); \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_CIterator_GetValue);    \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_CIterator_GetKey);      \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptDictionary_CIterator_operator_PP); \
    IMPORT_FUNCTION_DEFINE(ASEXT_GetCurrentContext);                       \
    IMPORT_FUNCTION_DEFINE(ASEXT_CScriptBuilder_DefineWord);               \
    IMPORT_FUNCTION_DEFINE(ASEXT_CASBaseManager_GetTypeInfoByName);        \
    IMPORT_FUNCTION_POINTER_DEFINE(ASEXT_CallHook);                        \
    IMPORT_FUNCTION_POINTER_DEFINE(ASEXT_CallCASBaseCallable);

#define REGISTER_PLAIN_VALUE_OBJECT(name)                                                                                                                                  \
    ASEXT_RegisterObjectType(pASDoc, #name " plain value object", #name, sizeof(name), asOBJ_VALUE);                                                                       \
    ASEXT_RegisterObjectBehaviour(pASDoc, "Default constructor", #name, ObjectBehaviour_Constructor, "void " #name "()", (void*)name##_ctor, 4);                           \
    ASEXT_RegisterObjectBehaviour(pASDoc, "Copy constructor", #name, ObjectBehaviour_Constructor, "void " #name "(const " #name "& in other)", (void*)name##_copyctor, 4); \
    ASEXT_RegisterObjectBehaviour(pASDoc, "Destructor", #name, ObjectBehaviour_Destructor, "void Destruct" #name "()", (void*)name##_dtor, 4);                             \
    ASEXT_RegisterObjectMethod(pASDoc, "operator=", #name, #name "& opAssign(const " #name "& in other)", (void*)name##_opassign, 3);

#define EXTERN_PLAIN_VALUE_OBJECT(name)                       \
    void                 name##_ctor(name* pthis);            \
    void                 name##_copyctor(name* a1, name* a2); \
    void                 name##_dtor(name* pthis);            \
    name* SC_SERVER_DECL name##_opassign(name* a1, SC_SERVER_DUMMYARG name* a2);
