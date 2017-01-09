#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "script.h"

using namespace v8;

// A Script is created for every bit of JavaScript evaluated. This is so if an
// exception is thrown through JavaScript the stack trace can include the
// script's source. For each script, a filename is generated:
// {resource name or "javascript" if none}-{script id}
// The script name is stored in the script object. script_sources is a
// dictionary mapping script names to script source. Each frame in JavaScript
// in a stack trace has __name__ pointing to the script name and __loader__
// pointing to the object of ScriptLoader. ScriptLoader's get_source reads the
// source out of script_sources. When a script is created an entry is added to
// script_sources, when it is destroyed its entry is deleted from
// script_sources.

int script_loader_type_init();
Eternal<Context> compile_context;
PyObject *script_sources;
PyObject *script_loader;
PyObject *javascript;

PyTypeObject script_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int script_type_init() {
    IN_V8;
    compile_context.Set(isolate, Context::New(isolate));

    script_sources = PyDict_New();
    if (script_sources == NULL) return -1;

    javascript = PyUnicode_InternFromString("javascript");

    script_type.tp_name = "v8py.Script";
    script_type.tp_basicsize = sizeof(script_c);
    script_type.tp_flags = Py_TPFLAGS_DEFAULT;
    script_type.tp_doc = "";
    script_type.tp_new = (newfunc) script_new;
    script_type.tp_dealloc = (destructor) script_dealloc;
    if (PyType_Ready(&script_type) < 0) return -1;

    return script_loader_type_init();
}

PyObject *construct_script_name(Local<Value> js_name, int id) {
    PyObject *name;
    if (js_name.IsEmpty() || js_name->IsUndefined()) {
        name = javascript;
        Py_INCREF(name);
    } else {
        name = py_from_js(js_name, compile_context.Get(isolate));
        PyErr_PROPAGATE(name);
    }
    if (id == 0) {
        return name;
    }
    PyObject *filename = PyUnicode_FromFormat("%S-%d", name, id);
    Py_DECREF(name);
    return filename;
}

PyObject *script_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    IN_V8;
    IN_CONTEXT(compile_context.Get(isolate));
    JS_TRY

    PyObject *source;
    if (PyArg_ParseTuple(args, "O", &source) < 0) {
        return NULL;
    }
    if (!PyString_Check(source)) {
        PyErr_SetString(PyExc_TypeError, "source must be a string");
        return NULL;
    }

    script_c *self = (script_c *) type->tp_alloc(type, 0);

    ScriptCompiler::Source js_source(js_from_py(source, context).As<String>());
    MaybeLocal<UnboundScript> maybe_script = ScriptCompiler::CompileUnbound(isolate, &js_source);
    PY_PROPAGATE_JS;
    Local<UnboundScript> script = maybe_script.ToLocalChecked();
    self->script.Reset(isolate, script);

    PyObject *filename = construct_script_name(script->GetScriptName(), script->GetId());
    PyErr_PROPAGATE(filename);
    if (PyDict_SetItem(script_sources, filename, source) < 0) return NULL;

    return (PyObject *) self;
}

void script_dealloc(script_c *self) {
    self->script.Reset();
    Py_DECREF(self->source);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyObject *script_loader_get_source(PyObject *self, PyObject *name);
typedef struct {
    PyObject_HEAD
} script_loader_c;
PyTypeObject script_loader_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
PyMethodDef script_loader_methods[] = {
    {"get_source", (PyCFunction) script_loader_get_source, METH_O, NULL},
    {NULL},
};
int script_loader_type_init() {
    script_loader_type.tp_name = "v8py.ScriptLoader";
    script_loader_type.tp_basicsize = sizeof(script_loader_c);
    script_loader_type.tp_flags = Py_TPFLAGS_DEFAULT;
    script_loader_type.tp_methods = script_loader_methods;
    if (PyType_Ready(&script_loader_type) < 0) return -1;

    script_loader = script_loader_type.tp_alloc(&script_loader_type, 0);
    if (script_loader == NULL) return -1;
    return 0;
}

PyObject *script_loader_get_source(PyObject *self, PyObject *name) {
    // if this fails it returns NULL which we just propagate
    return PyDict_GetItem(script_sources, name);
}
