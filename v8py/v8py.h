#ifndef V8PY_H
#define V8PY_H

#ifdef _WIN32
// v8.h(1680): error C2059: syntax error: '('
#undef COMPILER
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

#include <v8.h>
#include "polyfill.h"
#include "exception.h"
#include "kappa.h"

using namespace v8;

extern Isolate *isolate;
extern PyObject *null_object;
#define STRING_BUFFER_SIZE 512
static uint16_t string_buffer[STRING_BUFFER_SIZE] = {};

#define NORETURN __attribute__ ((noreturn))

#define JSTR(str) String::NewFromUtf8(isolate, str, NewStringType::kNormal).ToLocalChecked()
#define JSDUMP(obj) \
    { \
        String::Utf8Value value(obj); \
        printf("%s\n", *value); \
    }

#define IN_V8 \
    Locker locker(isolate); \
    Isolate::Scope is(isolate); \
    USING_V8
#define ESCAPING_IN_V8 \
    Locker locker(isolate); \
    Isolate::Scope is(isolate); \
    ESCAPING_V8

#define USING_V8 \
    HandleScope hs(isolate)
#define ESCAPING_V8 \
    EscapableHandleScope hs(isolate)

#define IN_CONTEXT(ctx) \
    Local<Context> context = ctx; \
    Context::Scope cs(context);

#endif
