#include <Python.h>
#include <v8.h>
#include <pthread.h>

#include "v8py.h"
#include "context.h"
#include "convert.h"
#include "pyclass.h"

using namespace v8;

PyMethodDef context_methods[] = {
    {"eval", (PyCFunction) context_eval, METH_VARARGS | METH_KEYWORDS, NULL},
    {"expose", (PyCFunction) context_expose, METH_VARARGS | METH_KEYWORDS, NULL},
    {"expose_module", (PyCFunction) context_expose_module, METH_O, NULL},
    {"gc", (PyCFunction) context_gc, METH_NOARGS, NULL},
    {NULL},
};
// Python is wrong. The first entry is not modifiable and should be const char *
PyGetSetDef context_getset[] = {
    {(char *) "glob", (getter) context_get_global, NULL, NULL, NULL},
    {NULL},
};
PyMappingMethods context_mapping = {
    NULL, (binaryfunc) context_getitem, (objobjargproc) context_setitem
};
PyTypeObject context_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int context_type_init() {
    context_type.tp_name = "v8py.Context";
    context_type.tp_basicsize = sizeof(context);
    context_type.tp_flags = Py_TPFLAGS_DEFAULT;
    context_type.tp_doc = "";
    context_type.tp_dealloc = (destructor) context_dealloc;
    context_type.tp_new = (newfunc) context_new;
    context_type.tp_methods = context_methods;
    context_type.tp_getattro = (getattrofunc) context_getattro;
    context_type.tp_setattro = (setattrofunc) context_setattro;
    context_type.tp_getset = context_getset;
    context_type.tp_as_mapping = &context_mapping;
    return PyType_Ready(&context_type);
}

void context_dealloc(context *self) {
    self->js_context.Reset();
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    IN_V8;

    PyObject *global = NULL;
    if (PyArg_ParseTuple(args, "|O", &global) < 0) {
        return NULL;
    }
    if (global != NULL) {
        if (PyType_Check(global) || PyClass_Check(global)) {
            PyObject *no_args = PyTuple_New(0);
            PyErr_PROPAGATE(no_args);
            global = PyObject_Call(global, no_args, NULL);
            Py_DECREF(no_args);
            PyErr_PROPAGATE(global);
        } else {
            Py_INCREF(global);
        }
    }

    context *self = (context *) type->tp_alloc(type, 0);
    PyErr_PROPAGATE(self);

    MaybeLocal<ObjectTemplate> global_template;
    if (global != NULL) {
        PyObject *global_type;
        if (PyInstance_Check(global)) {
            global_type = PyObject_GetAttrString(global, "__class__");
        } else {
            global_type = (PyObject *) Py_TYPE(global);
            Py_INCREF(global_type);
        }
        py_class *templ;
        templ = (py_class *) py_class_to_template(global_type);
        Py_DECREF(global_type);
        global_template = templ->templ->Get(isolate)->InstanceTemplate();
    }

    IN_CONTEXT(Context::New(isolate, NULL, global_template));
    self->js_context.Reset(isolate, context);

    context->SetEmbedderData(OBJECT_PROTOTYPE_SLOT, Object::New(isolate)->GetPrototype());
    context->SetEmbedderData(ERROR_PROTOTYPE_SLOT, Exception::Error(String::Empty(isolate)).As<Object>()->GetPrototype());

    if (global != NULL) {
        py_class_init_js_object(context->Global()->GetPrototype().As<Object>(), global, context);
    }

    return (PyObject *) self;
}

PyObject *context_expose(context *self, PyObject *args, PyObject *kwargs) {
    IN_V8;
    Local<Context> context = self->js_context.Get(isolate);
    Local<Object> global = context->Global();

    args = PySequence_Fast(args, "sequence required");
    PyErr_PROPAGATE(args);
    for (int i = 0; i < PySequence_Fast_GET_SIZE(args); i++) {
        PyObject *object = PySequence_Fast_GET_ITEM(args, i);
        PyErr_PROPAGATE(object);
        if (!PyObject_HasAttrString(object, "__name__")) {
            PyErr_SetString(PyExc_TypeError, "Object passed to expose must have a __name__");
            return NULL;
        }
    }
    for (int i = 0; i < PySequence_Fast_GET_SIZE(args); i++) {
        PyObject *object = PySequence_Fast_GET_ITEM(args, i);
        PyErr_PROPAGATE(object);
        PyObject *name = PyObject_GetAttrString(object, "__name__");
        PyErr_PROPAGATE(name);
        global->CreateDataProperty(context, js_from_py(name, context).As<String>(), js_from_py(object, context));
    }
    Py_DECREF(args);

    if (kwargs != NULL) {
        PyObject *name, *object;
        Py_ssize_t pos = 0;
        while (PyDict_Next(kwargs, &pos, &name, &object)) {
            global->CreateDataProperty(context, js_from_py(name, context).As<String>(), js_from_py(object, context));
        }
    }

    Py_RETURN_NONE;
}

PyObject *context_expose_module(context *self, PyObject *module) {
    if (!PyModule_Check(module)) {
        PyErr_SetString(PyExc_TypeError, "context_expose_module requires a module");
        return NULL;
    }

    PyObject *module_all_slow = PyObject_Dir(module);
    PyErr_PROPAGATE(module_all_slow);
    PyObject *module_all = PySequence_Fast(module_all_slow, "o noes");
    Py_DECREF(module_all_slow);
    PyErr_PROPAGATE(module_all);
    PyObject *members = PyDict_New();
    PyErr_PROPAGATE(members);
    for (int i = 0; i < PySequence_Fast_GET_SIZE(module_all); i++) {
        PyObject *name = PySequence_Fast_GET_ITEM(module_all, i);
        if (!PyString_StartsWithString(name, "_")) {
            PyObject *value = PyObject_GetAttr(module, name);
            if (value == NULL) {
                Py_DECREF(members);
                return NULL;
            }
            if (PyDict_SetItem(members, name, value) < 0) {
                Py_DECREF(members);
                return NULL;
            }
        }
    }

    PyObject *no_args = PyTuple_New(0);
    PyErr_PROPAGATE(no_args);
    PyObject *result = context_expose(self, no_args, members);
    Py_DECREF(no_args);
    Py_DECREF(members);
    return result;
}

void *breaker_thread(void *param) {
    double timeout = *(double *) param;
    usleep((int) (timeout * 1000000));
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    isolate->TerminateExecution();
    return NULL;
}

PyObject *context_eval(context *self, PyObject *args, PyObject *kwargs) {
    PyObject *program;
    double timeout = 0;
    static char *keywords[] = {"program", "timeout", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|d", keywords, &program, &timeout) < 0) {
        return NULL;
    }
    if (!PyString_Check(program)) {
        PyErr_SetString(PyExc_TypeError, "program must be a string");
        return NULL;
    }

    IN_V8;
    IN_CONTEXT(self->js_context.Get(isolate));
    JS_TRY

    MaybeLocal<Script> script = Script::Compile(context, js_from_py(program, context).As<String>());
    PY_PROPAGATE_JS;

    pthread_t breaker_id;
    if (timeout > 0) {
        errno = pthread_create(&breaker_id, NULL, breaker_thread, &timeout);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    MaybeLocal<Value> result = script.ToLocalChecked()->Run(context);

    if (timeout > 0) {
        pthread_cancel(breaker_id);
        errno = pthread_join(breaker_id, NULL);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    PY_PROPAGATE_JS;
    return py_from_js(result.ToLocalChecked(), context);
}

PyObject *context_get_global(context *self, void *shit) {
    IN_V8;
    Local<Context> context = self->js_context.Get(isolate);
    return py_from_js(context->Global()->GetPrototype(), context);
}

PyObject *context_getattro(context *self, PyObject *name) {
    PyObject *value = PyObject_GenericGetAttr((PyObject *) self, name);
    if (value == NULL) {
        PyErr_Clear();
        return context_getitem(self, name);
    }
    return value;
}
PyObject *context_getitem(context *self, PyObject *name) {
    PyObject *global = context_get_global(self, NULL);
    PyErr_PROPAGATE(global);
    return PyObject_GetAttr(context_get_global(self, NULL), name);
}

int context_setattro(context *self, PyObject *name, PyObject *value) {
    // use GenericGetAttr to find out if Context defines the property
    PyObject *ctx_value = PyObject_GenericGetAttr((PyObject *) self, name);
    if (ctx_value == NULL) {
        // if the property is not defined by Context, set it on the global
        PyErr_Clear();
        return context_setitem(self, name, value);
    } else {
        Py_DECREF(ctx_value);
    }
    // if the property is defined by Context, delegate
    return PyObject_GenericSetAttr((PyObject *) self, name, value);
}
int context_setitem(context *self, PyObject *name, PyObject *value) {
    PyObject *global = context_get_global(self, NULL);
    PyErr_PROPAGATE_(global);
    return PyObject_SetAttr(global, name, value);
}

PyObject *context_gc(context *self) {
    IN_V8;
    isolate->RequestGarbageCollectionForTesting(Isolate::GarbageCollectionType::kFullGarbageCollection);
    Py_RETURN_NONE;
}

