#include "polyfill.h"
#include <v8.h>

using namespace v8;

extern Isolate *isolate;
extern PyObject *null_object;

#define NORETURN __attribute__ ((noreturn))

#undef assert // silence "macro redefined"
#define assert(condition) \
    if (condition) \
        (void) NULL; \
    else \
        assert_failed(#condition, __FILE__, __LINE__)

NORETURN void assert_failed(const char *condition, const char *file, int line);

#define JSTR(str) String::NewFromUtf8(isolate, str, NewStringType::kNormal).ToLocalChecked()

