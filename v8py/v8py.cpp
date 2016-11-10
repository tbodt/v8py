#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Python.h>
#include <v8.h>
#include <libplatform/libplatform.h>

#include "v8py.h"
#include "context.h"
#include "classtemplate.h"
#include "jsobject.h"

using namespace v8;

class MallocAllocator : public ArrayBuffer::Allocator {
    public:
    virtual void *Allocate(size_t size) {
        void *data = AllocateUninitialized(size);
        if (data != NULL)
            memset(data, 0, size);
        return data;
    }

    virtual void *AllocateUninitialized(size_t size) {
        return malloc(size);
    }

    virtual void Free(void *data, size_t size) {
        free(data);
    }
};

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
        create_params.array_buffer_allocator = new MallocAllocator();
        isolate = Isolate::New(create_params);
    }
}

static PyMethodDef v8_methods[] = {
    {NULL},
};

PyObject *py_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    PyErr_SetString(PyExc_NotImplementedError, "this type can't be created from Python");
    return NULL;
}

PyMODINIT_FUNC initv8py() {
    initialize_v8();
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
    Py_INCREF(&py_class_type);
    PyModule_AddObject(module, "ClassTemplate", (PyObject *) &py_class_type);

    if (js_object_type_init() < 0)
        return;
    if (js_function_type_init() < 0)
        return;
}

