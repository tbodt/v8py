#include <v8.h>
#include "polyfill.h"
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
    do { \
        String::Utf8Value value(obj); \
        printf("%s\n", *value); \
    } while (0)

