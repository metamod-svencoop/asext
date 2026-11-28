#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <asext_api.h>
#include <GoldSrcExt/svencoop/server/angelscript_ext.h>

// Verify ABI layouts of Sven Co-op mirrored types on both 32-bit and 64-bit targets.
#if defined(_WIN64) || defined(__x86_64__) || defined(__LP64__)

static_assert(sizeof(void*) == 8, "Expected 64-bit build");
static_assert(offsetof(CString, m_ptr) == 24, "CString::m_ptr offset mismatch on 64-bit");
static_assert(offsetof(CString, m_len) == 32, "CString::m_len offset mismatch on 64-bit");
static_assert(offsetof(CString, m_capacity) == 36, "CString::m_capacity offset mismatch on 64-bit");
static_assert(offsetof(CString, m_tokenizeIdx) == 40, "CString::m_tokenizeIdx offset mismatch on 64-bit");
static_assert(sizeof(CString) == 48, "CString size mismatch on 64-bit");

static_assert(offsetof(CASBLOB, m_buf) == 8, "CASBLOB::m_buf offset mismatch on 64-bit");
static_assert(offsetof(CASBLOB, m_size) == 16, "CASBLOB::m_size offset mismatch on 64-bit");
static_assert(offsetof(CASBLOB, m_read_offset) == 20, "CASBLOB::m_read_offset offset mismatch on 64-bit");
static_assert(offsetof(CASBLOB, m_write_offset) == 24, "CASBLOB::m_write_offset offset mismatch on 64-bit");

static_assert(offsetof(CScriptArray, m_buffer) == 24, "CScriptArray::m_buffer offset mismatch on 64-bit");
static_assert(offsetof(CScriptArray, m_ElementTypeId) == 36, "CScriptArray::m_ElementTypeId offset mismatch on 64-bit");

static_assert(offsetof(CASServerManager, scriptEngine) == 24, "CASServerManager::scriptEngine offset mismatch on 64-bit");
static_assert(offsetof(CASServerManager, curModule) == 48, "CASServerManager::curModule offset mismatch on 64-bit");

static_assert(sizeof(CScriptDictionary_CIterator) == 16, "CScriptDictionary_CIterator size mismatch on 64-bit");

#else

static_assert(sizeof(void*) == 4, "Expected 32-bit build");
static_assert(offsetof(CString, m_ptr) == 20, "CString::m_ptr offset mismatch on 32-bit");
static_assert(offsetof(CString, m_len) == 24, "CString::m_len offset mismatch on 32-bit");
static_assert(offsetof(CString, m_capacity) == 28, "CString::m_capacity offset mismatch on 32-bit");
static_assert(offsetof(CString, m_tokenizeIdx) == 32, "CString::m_tokenizeIdx offset mismatch on 32-bit");
static_assert(sizeof(CString) == 36, "CString size mismatch on 32-bit");

static_assert(offsetof(CASBLOB, m_buf) == 4, "CASBLOB::m_buf offset mismatch on 32-bit");
static_assert(offsetof(CASBLOB, m_size) == 8, "CASBLOB::m_size offset mismatch on 32-bit");
static_assert(offsetof(CASBLOB, m_read_offset) == 12, "CASBLOB::m_read_offset offset mismatch on 32-bit");
static_assert(offsetof(CASBLOB, m_write_offset) == 16, "CASBLOB::m_write_offset offset mismatch on 32-bit");

static_assert(offsetof(CScriptArray, m_buffer) == 16, "CScriptArray::m_buffer offset mismatch on 32-bit");
static_assert(offsetof(CScriptArray, m_ElementTypeId) == 24, "CScriptArray::m_ElementTypeId offset mismatch on 32-bit");

static_assert(offsetof(CASServerManager, scriptEngine) == 12, "CASServerManager::scriptEngine offset mismatch on 32-bit");
static_assert(offsetof(CASServerManager, curModule) == 24, "CASServerManager::curModule offset mismatch on 32-bit");

static_assert(sizeof(CScriptDictionary_CIterator) == 8, "CScriptDictionary_CIterator size mismatch on 32-bit");

#endif

static_assert(sizeof(CScriptDictionary_CIterator) == sizeof(goldsrcext::svencoop::ASDictIteratorStorage),
              "CScriptDictionary_CIterator must match ASDictIteratorStorage");

static int SC_SERVER_DECL SampleMethod(void* /*pthis*/ SC_SERVER_DUMMYARG_NOCOMMA, int a, int b)
{
    return a + b;
}

int main()
{
    // Verify SC_SERVER_DECL / SC_SERVER_DUMMYARG calling convention on this architecture.
#if defined(_WIN32) && !defined(_WIN64)
    const int sum = SampleMethod(nullptr, 0, 17, 25);
#else
    using PlainFn = int (*)(void*, int, int);
    PlainFn   fn  = &SampleMethod;
    const int sum = fn(nullptr, 17, 25);
#endif
    if (sum != 42)
    {
        std::fprintf(stderr, "SampleMethod returned %d, expected 42\n", sum);
        return 1;
    }

    // Verify CString aggregate zero-initialization still compiles cleanly.
    CString s = {0};
    if (!s.empty() || s.size() != 0 || s.c_str()[0] != '\0')
    {
        std::fprintf(stderr, "Zero-initialized CString check failed\n");
        return 1;
    }

    std::printf("mmfg_test_asext_abi passed.\n");
    return 0;
}
