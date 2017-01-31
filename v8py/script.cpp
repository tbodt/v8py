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
// The script name is stored in the script object. scripts_by_name is a
// dictionary mapping script names to script source. Each frame in JavaScript
// in a stack trace has __name__ pointing to the script name and __loader__
// pointing to the object of ScriptLoader. ScriptLoader's get_source reads the
// source out of scripts_by_name. When a script is created an entry is added to
// scripts_by_name, when it is destroyed its entry is deleted from
// scripts_by_name.

int script_loader_type_init();
Eternal<Context> compile_context;
PyObject *scripts_by_name;
PyObject *script_loader;
PyObject *javascript;

PyTypeObject script_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int script_type_init() {
    IN_V8;
    compile_context.Set(isolate, Context::New(isolate));

    PyObject *weakref_module = PyImport_ImportModule("weakref");
    PyErr_PROPAGATE_(weakref_module);
    PyObject *weak_value_dict = PyObject_GetAttrString(weakref_module, "WeakValueDictionary");
    PyErr_PROPAGATE_(weak_value_dict);
    scripts_by_name = PyObject_CallObject(weak_value_dict, NULL);
    PyErr_PROPAGATE_(scripts_by_name);

    javascript = PyString_InternFromString("javascript");

    script_type.tp_name = "v8py.Script";
    script_type.tp_basicsize = sizeof(script_c);
    script_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_WEAKREFS;
    script_type.tp_weaklistoffset = offsetof(script_c, weakrefs);
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
    PyObject *script_name = PyUnicode_FromFormat("%S-%d", name, id);
    Py_DECREF(name);
    return script_name;
}

PyObject *script_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    IN_V8;
    IN_CONTEXT(compile_context.Get(isolate));
    JS_TRY

    static const char *keywords[] = {"source", "filename", NULL};
    PyObject *source;
    PyObject *filename = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", (char **) keywords, &source, &filename) < 0) {
        return NULL;
    }
    if (!PyString_Check(source)) {
        PyErr_SetString(PyExc_TypeError, "source must be a string");
        return NULL;
    }
    if (filename != Py_None && !PyString_Check(filename)) {
        PyErr_SetString(PyExc_TypeError, "filename must be a string or None");
        return NULL;
    }

    MaybeLocal<UnboundScript> maybe_script = script_compile(context, source, filename);
    PY_PROPAGATE_JS;
    Local<UnboundScript> script = maybe_script.ToLocalChecked();
    PyObject *script_name = construct_script_name(script->GetScriptName(), script->GetId());
    PyErr_PROPAGATE(script_name);
    if (PySequence_Contains(scripts_by_name, script_name)) {
        return PyObject_GetItem(scripts_by_name, script_name);
    }

    script_c *self = (script_c *) type->tp_alloc(type, 0);
    self->script.Reset(isolate, script);
    self->script_name = script_name;
    Py_INCREF(source);
    self->source = source;

    if (PyObject_SetItem(scripts_by_name, self->script_name, (PyObject *) self) < 0) return NULL;

    return (PyObject *) self;
}

MaybeLocal<UnboundScript> script_compile(Local<Context> context, PyObject *source, PyObject *filename) {
    EscapableHandleScope hs(isolate);
    MaybeLocal<UnboundScript> maybe_script;
    // fucking god / c++
    if (filename != Py_None) {
        ScriptOrigin origin(js_from_py(filename, context));
        ScriptCompiler::Source js_source(js_from_py(source, context).As<String>(), origin);
        maybe_script = ScriptCompiler::CompileUnbound(isolate, &js_source);
    } else {
        ScriptCompiler::Source js_source(js_from_py(source, context).As<String>());
        maybe_script = ScriptCompiler::CompileUnbound(isolate, &js_source);
    }
    if (maybe_script.IsEmpty())
        return maybe_script;
    return hs.Escape(maybe_script.ToLocalChecked());
}

void script_dealloc(script_c *self) {
    self->script.Reset();

    PyObject_DelItem(scripts_by_name, self->script_name); // can't do anything if this fails

    Py_DECREF(self->script_name);
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
    script_c *script = (script_c *) PyObject_GetItem(scripts_by_name, name);
    PyErr_PROPAGATE(script);
    Py_INCREF(script->source);
    return script->source;
}
