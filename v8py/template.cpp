#include <stdio.h>
#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "context.h"
#include "convert.h"
#include "template.h"

PyTypeObject py_template_type = {
    PyObject_HEAD_INIT(NULL)
};

int py_template_type_init() {
    py_template_type.tp_name = "v8py.FunctionTemplate";
    py_template_type.tp_basicsize = sizeof(py_template);
    py_template_type.tp_dealloc = (destructor) py_template_dealloc;
    py_template_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_template_type.tp_doc = "";
    py_template_type.tp_new = py_fake_new;
    return PyType_Ready(&py_template_type);
}

static void py_template_callback(const FunctionCallbackInfo<Value> &info);

PyObject *py_template_new(PyObject *function) {
    py_template *self = (py_template *) py_template_type.tp_alloc(&py_template_type, 0);
    if (self == NULL) {
        return NULL;
    }
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

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    Local<External> js_self = External::New(isolate, self);
    Local<FunctionTemplate> js_template = FunctionTemplate::New(isolate, py_template_callback, js_self);
    self->js_template->Reset(isolate, js_template);

    return (PyObject *) self;
}

static PyObject *template_dict = NULL;

PyObject *py_function_to_template(PyObject *func) {
    if (template_dict == NULL) {
        template_dict = PyDict_New();
        if (template_dict == NULL) {
            return NULL;
        }
    }

    PyObject *templ = PyDict_GetItem(template_dict, func);
    if (templ != NULL) {
        Py_INCREF(templ);
        return templ;
    }

    templ = py_template_new(func);
    PyDict_SetItem(template_dict, func, templ);
    return templ;
}

Local<Function> py_template_to_function(py_template *self, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Function> function = self->js_template->Get(isolate)->GetFunction(context).ToLocalChecked();
    function->SetName(js_from_py(self->function_name, context).As<String>());
    return hs.Escape(function);
}

static void py_template_callback(const FunctionCallbackInfo<Value> &info) {
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    py_template *self = (py_template *) info.Data().As<External>()->Value();
    PyObject *args = pys_from_jss(info, context);
    PyObject *result = PyObject_CallObject(self->function, args);
    if (result == NULL) {
        // function threw an exception
        // TODO throw an actual javascript exception
        printf("exception was thrown");
        Py_INCREF(Py_None);
        result = Py_None;
    }
    Py_DECREF(args);
    Local<Value> js_result = js_from_py(result, context);
    Py_DECREF(result);
    info.GetReturnValue().Set(js_result);
}

void py_template_dealloc(py_template *self) {
    printf("this should never happen\n");
    (void) *((char *)NULL); // intentional segfault
}

