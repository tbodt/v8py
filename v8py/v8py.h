#ifndef V8PY_H
#define V8PY_H

#include <v8.h>
#include "polyfill.h"
#include "exception.h"
#include "kappa.h"

using namespace v8;

extern Isolate *isolate;
extern PyObject *null_object;

#define NORETURN __attribute__ ((noreturn))

#undef assert // shut up "macro redefined" warning
#define assert(condition) \
    if (condition) \
        (void) NULL; \
    else \
        assert_failed(#condition, __FILE__, __LINE__)

NORETURN void assert_failed(const char *condition, const char *file, int line);

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
