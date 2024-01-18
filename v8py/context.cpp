#include <Python.h>
#include "v8py.h"
#include <v8.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include "context.h"
#include "script.h"
#include "convert.h"
#include "jsobject.h"
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
    {(char *) "timeout", (getter) context_get_timeout, (setter) context_set_timeout, NULL, NULL},
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
    context_type.tp_basicsize = sizeof(context_c);
    context_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
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

PyObject *context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    IN_V8;

    double timeout = 0;
    static const char *keywords[] = {"global", "timeout", NULL};

    PyObject *global = NULL;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|Od", (char **) keywords, &global, &timeout) < 0) {
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

    context_c *self = (context_c *) type->tp_alloc(type, 0);
    self->has_debugger = false;
    self->timeout = timeout;
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

    context->SetEmbedderData(CONTEXT_OBJECT_SLOT, External::New(isolate, self));
    context->SetEmbedderData(OBJECT_PROTOTYPE_SLOT, Object::New(isolate)->GetPrototype());
    context->SetEmbedderData(ERROR_PROTOTYPE_SLOT, Exception::Error(String::Empty(isolate)).As<Object>()->GetPrototype());

    self->js_object_cache = PyDict_New();
    PyErr_PROPAGATE(self->js_object_cache);

    self->scripts = PySet_New(NULL);
    PyErr_PROPAGATE(self->scripts);

    if (global != NULL) {
        py_class_init_js_object(context->Global()->GetPrototype().As<Object>(), global, context);
    }

    return (PyObject *) self;
}

void context_dealloc(context_c *self) {
    self->js_context.Reset();
    PyDict_Clear(self->js_object_cache);
    Py_DECREF(self->js_object_cache);
    Py_DECREF(self->scripts);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *context_expose(context_c *self, PyObject *args, PyObject *kwargs) {
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

PyObject *context_expose_module(context_c *self, PyObject *module) {
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

#ifdef _WIN32

UINT s_timer_id;

static void CALLBACK breaker_callback(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
    isolate->TerminateExecution();
}

#else

pthread_t breaker_id;
useconds_t s_timeout;

void *breaker_thread(void *param) {
    useconds_t timeout = *(useconds_t *) param;
    usleep(timeout);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    isolate->TerminateExecution();
    return NULL;
}
#endif

static double context_timeout(Local<Context> context) {
    context_c *ctx_c = (context_c *) context->GetEmbedderData(CONTEXT_OBJECT_SLOT).As<External>()->Value();
    return ctx_c->timeout;
}

static bool setup_timeout(double timeout) {
#ifdef _WIN32
    if (timeout > 0) {
        UINT timeout_ = (UINT) (timeout * 1000);
        s_timer_id = timeSetEvent(timeout_, 0, (LPTIMECALLBACK) breaker_callback, 0,
            TIME_ONESHOT | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
        if (s_timer_id == NULL) {
            PyErr_SetFromErrno(PyExc_OSError);
            return false;
        }
    }
#else
    if (timeout > 0) {
        s_timeout = (useconds_t) (timeout * 1000000);
        errno = pthread_create(&breaker_id, NULL, breaker_thread, &s_timeout);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return false;
        }
    }
#endif
    return true;
}

static bool cleanup_timeout(double timeout) {
#ifdef _WIN32
    if (timeout > 0 && s_timer_id != NULL) {
        timeKillEvent(s_timer_id);
    }
#else
    if (timeout > 0) {
        pthread_cancel(breaker_id);
        errno = pthread_join(breaker_id, NULL);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            return false;
        }
    }
#endif
    return true;
}

bool context_setup_timeout(Local<Context> context) {
    return setup_timeout(context_timeout(context));
}
bool context_cleanup_timeout(Local<Context> context) {
    return cleanup_timeout(context_timeout(context));
}

PyObject *context_eval(context_c *self, PyObject *args, PyObject *kwargs) {
    PyObject *program;
    PyObject *filename = Py_None;
    double timeout = self->timeout;
    static const char *keywords[] = {"program", "timeout", "filename", NULL};
    // python needs to fix their shit and make it const
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|dO", (char **) keywords, &program, &timeout, &filename) < 0) {
        return NULL;
    }
    if (!PyString_Check(program) && !PyObject_TypeCheck(program, &script_type)) {
        PyErr_SetString(PyExc_TypeError, "program must be a string or Script");
        return NULL;
    }

    if (PyString_Check(program)) {
        program = PyObject_CallFunctionObjArgs((PyObject *) &script_type, program, filename, NULL);
        PyErr_PROPAGATE(program);
    } else {
        Py_INCREF(program);
    }
    assert(PyObject_TypeCheck(program, &script_type));
    script_c *py_script = (script_c *) program;

    IN_V8;
    IN_CONTEXT(self->js_context.Get(isolate));
    JS_TRY

    PySet_Add(self->scripts, program);
    Local<UnboundScript> unbound_script;
    if (self->has_debugger) {
        MaybeLocal<UnboundScript> maybe_script = script_compile(context, py_script->source, py_script->script_name);
        PY_PROPAGATE_JS;
        unbound_script = maybe_script.ToLocalChecked();
    } else {
        unbound_script = py_script->script.Get(isolate);
    }
    Py_DECREF(program);
    Local<Script> script = unbound_script->BindToCurrentContext();

    if (!setup_timeout(timeout)) return NULL;
    MaybeLocal<Value> result = script->Run(context);
    if (!cleanup_timeout(timeout)) return NULL;

    PY_PROPAGATE_JS;
    return py_from_js(result.ToLocalChecked(), context);
}

Local<Object> context_get_cached_jsobject(Local<Context> js_context, PyObject *py_object) {
    EscapableHandleScope hs(isolate);
    context_c *self = (context_c *) js_context->GetEmbedderData(CONTEXT_OBJECT_SLOT).As<External>()->Value();
    if (PyMapping_HasKey(self->js_object_cache, py_object)) {
        js_object *jsobj = (js_object *) PyObject_GetItem(self->js_object_cache, py_object);
        if (jsobj == NULL) {
            // fuck
            PyErr_WriteUnraisable(PyString_InternFromString("v8py py_class_create_js_object getitem"));
        }
        return hs.Escape(jsobj->object.Get(isolate));
    }
    return Local<Object>();
}

void context_set_cached_jsobject(Local<Context> js_context, PyObject *py_object, Local<Object> object) {
    EscapableHandleScope hs(isolate);
    context_c *self = (context_c *) js_context->GetEmbedderData(CONTEXT_OBJECT_SLOT).As<External>()->Value();
    js_object *jsobj = js_object_weak_new(object, js_context);
    if (PyObject_SetItem(self->js_object_cache, py_object, (PyObject *) jsobj) < 0) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            // if it's a type error, it's probably "cannot create weak reference" and should be ignored.
            PyErr_Clear();
        } else {
            PyErr_WriteUnraisable(PyString_InternFromString("v8py py_class_create_js_object setitem"));
        }
    }
}

PyObject *context_get_current(PyObject *shit, PyObject *fuck) {
    Local<Context> current_context = isolate->GetCurrentContext();
    if (current_context.IsEmpty()) {
        Py_RETURN_NONE;
    }
    PyObject *context = (PyObject *) current_context->GetEmbedderData(CONTEXT_OBJECT_SLOT).As<External>()->Value();
    Py_INCREF(context);
    return context;
}

PyObject *context_get_timeout(context_c *self, void *shit) {
    return PyFloat_FromDouble(self->timeout);
}

int *context_set_timeout(context_c *self, PyObject *value, void *shit) {
    self->timeout = PyFloat_AsDouble(value);
    return 0;
}

PyObject *context_get_global(context_c *self, void *shit) {
    IN_V8;
    Local<Context> context = self->js_context.Get(isolate);
    return py_from_js(context->Global()->GetPrototype(), context);
}

PyObject *context_getattro(context_c *self, PyObject *name) {
    PyObject *value = PyObject_GenericGetAttr((PyObject *) self, name);
    if (value == NULL) {
        PyErr_Clear();
        return context_getitem(self, name);
    }
    return value;
}
PyObject *context_getitem(context_c *self, PyObject *name) {
    PyObject *global = context_get_global(self, NULL);
    PyErr_PROPAGATE(global);
    return PyObject_GetAttr(context_get_global(self, NULL), name);
}

int context_setattro(context_c *self, PyObject *name, PyObject *value) {
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
int context_setitem(context_c *self, PyObject *name, PyObject *value) {
    PyObject *global = context_get_global(self, NULL);
    PyErr_PROPAGATE_(global);
    return PyObject_SetAttr(global, name, value);
}

PyObject *context_gc(context_c *self) {
    IN_V8;
    isolate->RequestGarbageCollectionForTesting(Isolate::GarbageCollectionType::kFullGarbageCollection);
    Py_RETURN_NONE;
}

