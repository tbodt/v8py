#include <Python.h>
#include <v8.h>
#include <libplatform/libplatform.h>

#include "v8py.h"
#include "greenstack.h"
#include "context.h"
#include "script.h"
#include "pyclass.h"
#include "jsobject.h"
#include "debugger.h"
#include "utils.h"

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
        isolate->SetCaptureStackTraceForUncaughtExceptions(true, 100, 
                // sadly the v8 people screwed up and require me to cast this into to an enum
                static_cast<StackTrace::StackTraceOptions>(StackTrace::kOverview | StackTrace::kScriptId));
    }
}

PyObject *mark_hidden(PyObject *shit, PyObject *thing) {
    if (PyObject_SetAttrString(thing, "__v8py_hidden__", Py_True) < 0) {
        return NULL;
    }
    Py_INCREF(thing);
    return thing;
}

PyObject *mark_unconstructable(PyObject *shit, PyObject *thing) {
    if (PyObject_SetAttrString(thing, "__v8py_unconstructable__", Py_True) < 0) {
        return NULL;
    }
    Py_INCREF(thing);
    return thing;
}

static PyMethodDef v8_methods[] = {
    {"hidden", mark_hidden, METH_O, ""},
    {"unconstructable", mark_unconstructable, METH_O, ""},
    {"current_context", context_get_current, METH_NOARGS, ""},
    {"new", construct_new_object, METH_VARARGS, "Creates a new JavaScript object from a given constructor function"},
    {NULL},
};

PyObject *null_object = NULL;
typedef struct {PyObject_HEAD} null_t;
PyObject *null_repr(PyObject *self) {
    static PyObject *repr = NULL;
    if (repr == NULL) {
        repr = PyString_InternFromString("Null");
    }
    Py_INCREF(repr);
    return repr;
}
int null_bool(PyObject *self) {
    return false;
}
PyTypeObject null_type = {PyVarObject_HEAD_INIT(NULL, 0)};
PyNumberMethods null_as_number;
int null_type_init() {
    null_type.tp_name = "v8py.NullType";
    null_type.tp_basicsize = sizeof(null_t);
    null_type.tp_flags = Py_TPFLAGS_DEFAULT;
    null_as_number.nb_nonzero = null_bool;
    null_type.tp_as_number = &null_as_number;
    null_type.tp_repr = null_repr;
    null_type.tp_doc = "";
    if (PyType_Ready(&null_type) < 0)
        return -1;
    null_object = null_type.tp_alloc(&null_type, 0);
    return 0;
}

#if PY_MAJOR_VERSION < 3
#define FAIL
#else
#define FAIL NULL
#endif

#if PY_MAJOR_VERSION < 3
PyMODINIT_FUNC init_v8py() {
#else
PyMODINIT_FUNC PyInit__v8py() {
#endif
    initialize_v8();
    create_memes_plz_thx();

#if PY_MAJOR_VERSION < 3
    PyObject *module = Py_InitModule("_v8py", v8_methods);
#else
    static struct PyModuleDef v8_module_def = {PyModuleDef_HEAD_INIT};
    v8_module_def.m_name = "_v8py";
    v8_module_def.m_size = -1;
    v8_module_def.m_methods = v8_methods;
    PyObject *module = PyModule_Create(&v8_module_def);
#endif
    if (module == NULL) return FAIL;

    if (greenstack_init() < 0) return FAIL;

    if (context_type_init() < 0) return FAIL;
    Py_INCREF(&context_type);
    PyModule_AddObject(module, "Context", (PyObject *) &context_type);

    if (script_type_init() < 0) return FAIL;
    Py_INCREF(&script_type);
    PyModule_AddObject(module, "Script", (PyObject *) &script_type);

    if (debugger_type_init() < 0) return FAIL;
    Py_INCREF(&debugger_type);
    PyModule_AddObject(module, "Debugger", (PyObject *) &debugger_type);

    if (py_function_type_init() < 0) return FAIL;
    if (py_class_type_init() < 0) return FAIL;

    if (js_object_type_init() < 0) return FAIL;
    Py_INCREF(&js_object_type);
    PyModule_AddObject(module, "JSObject", (PyObject *) &js_object_type);

    if (js_function_type_init() < 0) return FAIL;
    Py_INCREF(&js_function_type);
    PyModule_AddObject(module, "JSFunction", (PyObject *) &js_function_type);

    if (js_exception_type_init() < 0) return FAIL;
    Py_INCREF(&js_exception_type);
    PyModule_AddObject(module, "JSException", (PyObject *) &js_exception_type);

    if (js_terminated_type_init() < 0) return FAIL;
    Py_INCREF(&js_terminated_type);
    PyModule_AddObject(module, "JavaScriptTerminated", (PyObject *) &js_terminated_type);

    if (null_type_init() < 0) return FAIL;
    Py_INCREF(null_object);
    PyModule_AddObject(module, "Null", null_object);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

NORETURN void assert_failed(const char *condition, const char *file, int line) {
    fprintf(stderr, "assert(%s) %s:%d\n", condition, file, line);
    abort();
}
