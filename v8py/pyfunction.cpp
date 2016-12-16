#include <stdio.h>
#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "context.h"
#include "convert.h"
#include "pyfunction.h"

PyTypeObject py_function_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};

int py_function_type_init() {
    py_function_type.tp_name = "v8py.Function";
    py_function_type.tp_basicsize = sizeof(py_function);
    py_function_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_function_type.tp_doc = "";
    return PyType_Ready(&py_function_type);
}

static void py_function_callback(const FunctionCallbackInfo<Value> &info);

PyObject *py_function_new(PyObject *function) {
    IN_V8;

    py_function *self = (py_function *) py_function_type.tp_alloc(&py_function_type, 0);
    PyErr_PROPAGATE(self);
    self->js_template = new Persistent<FunctionTemplate>();
    Py_INCREF(function);
    self->function = function;
    self->function_name = PyObject_GetAttrString(function, "__name__");

    // I've discovered that v8 trades memory leaks for speed. If you allocate a
    // FunctionTemplate and instantiate it, the FunctionTemplate, callback
    // data, and instantiated Function will **never** get GC'd. Presumably,
    // this is by design. This means that the FunctionTemplate python object
    // should never be freed either.
    Py_INCREF(self);

    Local<External> js_self = External::New(isolate, self);
    Local<FunctionTemplate> js_template = FunctionTemplate::New(isolate, py_function_callback, js_self);
    self->js_template->Reset(isolate, js_template);

    return (PyObject *) self;
}

static PyObject *template_dict = NULL;

PyObject *py_function_to_template(PyObject *func) {
    if (template_dict == NULL) {
        template_dict = PyDict_New();
        PyErr_PROPAGATE(template_dict);
    }

    PyObject *templ = PyDict_GetItem(template_dict, func);
    if (templ != NULL) {
        Py_INCREF(templ);
        return templ;
    }

    templ = py_function_new(func);
    PyDict_SetItem(template_dict, func, templ);
    Py_DECREF(templ);
    return templ;
}

Local<Function> py_template_to_function(py_function *self, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Function> function = self->js_template->Get(isolate)->GetFunction(context).ToLocalChecked();
    function->SetName(js_from_py(self->function_name, context).As<String>());
    return hs.Escape(function);
}

static void py_function_callback(const FunctionCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    py_function *self = (py_function *) info.Data().As<External>()->Value();
    PyObject *args = pys_from_jss(info, context);
    JS_PROPAGATE_PY(args);
    PyObject *result = PyObject_CallObject(self->function, args);
    JS_PROPAGATE_PY(result);
    Py_DECREF(args);
    Local<Value> js_result = js_from_py(result, context);
    Py_DECREF(result);
    info.GetReturnValue().Set(js_result);
}

