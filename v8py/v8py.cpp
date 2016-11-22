#include <Python.h>
#include <v8.h>
#include <libplatform/libplatform.h>

#include "v8py.h"
#include "context.h"
#include "pyclass.h"
#include "jsobject.h"
#include "exception.h"
#include "pydictionary.h"

using namespace v8;

static Platform *current_platform = NULL;
Isolate *isolate = NULL;
void initialize_v8() {
    if (current_platform == NULL) {
        V8::InitializeICU();
        current_platform = platform::CreateDefaultPlatform();
        V8::InitializePlatform(current_platform);
        V8::Initialize();
        // strlen is slow but that doesn't matter much here because this only happens once
        V8::SetFlagsFromString("--expose_gc", strlen("--expose_gc"));

        Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
        isolate = Isolate::New(create_params);
        isolate->SetCaptureStackTraceForUncaughtExceptions(true);
    }
}

static PyMethodDef v8_methods[] = {
    {NULL},
};

PyObject *null_object = NULL;
typedef struct {PyObject_HEAD} null_t;
PyTypeObject null_type = {PyObject_HEAD_INIT(NULL)};
int null_type_init() {
    null_type.tp_name = "v8py.NullType";
    null_type.tp_basicsize = sizeof(null_t);
    null_type.tp_flags = Py_TPFLAGS_DEFAULT;
    null_type.tp_doc = "";
    if (PyType_Ready(&null_type) < 0)
        return -1;
    null_object = null_type.tp_alloc(&null_type, 0);
    return 0;
}

PyMODINIT_FUNC initv8py() {
    initialize_v8();
    create_memes_plz_thx();

    PyObject *module = Py_InitModule("v8py", v8_methods);
    if (module == NULL)
        return;

    if (context_type_init() < 0)
        return;
    Py_INCREF(&context_type);
    PyModule_AddObject(module, "Context", (PyObject *) &context_type);

    if (py_function_type_init() < 0)
        return;
    if (py_class_type_init() < 0)
        return;
    py_dictionary_init();

    if (js_object_type_init() < 0)
        return;
    if (js_function_type_init() < 0)
        return;
    if (js_dictionary_type_init() < 0)
        return;
    if (js_exception_type_init() < 0)
        return;
    Py_INCREF(&js_exception_type);
    PyModule_AddObject(module, "JSException", (PyObject *) &js_exception_type);

    if (null_type_init() < 0)
        return;
    Py_INCREF(null_object);
    PyModule_AddObject(module, "Null", null_object);
}

NORETURN void assert_failed(const char *condition, const char *file, int line) {
    fprintf(stderr, "assert(%s) %s:%d\n", condition, file, line);
    abort();
}
