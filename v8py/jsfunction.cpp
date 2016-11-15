#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "jsobject.h"
#include "convert.h"

PyTypeObject js_function_type = {
    PyObject_HEAD_INIT(NULL)
};
int js_function_type_init() {
    js_function_type.tp_name = "v8py.BoundFunction";
    js_function_type.tp_basicsize = sizeof(js_function);
    js_function_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_function_type.tp_doc = "";
    js_function_type.tp_call = (ternaryfunc) js_function_call;
    js_function_type.tp_base = &js_object_type;
    return PyType_Ready(&js_function_type);
}

PyObject *js_function_call(js_function *self, PyObject *args, PyObject *kwargs) {
    HandleScope hs(isolate);
    Local<Context> context = self->object.context.Get(isolate);

    Local<Object> object = self->object.object.Get(isolate);
    Local<Value> js_this;
    if (self->js_this.IsEmpty()) {
        js_this = Undefined(isolate);
    } else {
        js_this = self->js_this.Get(isolate);
    }
    int argc;
    Local<Value> *argv = pys_from_jss(args, context, &argc);
    Local<Value> result = object->CallAsFunction(context, js_this, argc, argv).ToLocalChecked();
    return py_from_js(result, context);
}

void js_object_dealloc(js_object *self) {
    self->object.Reset();
    self->context.Reset();
    self->ob_type->tp_free((PyObject *) self);
}

void js_function_dealloc(js_function *self) {
    self->js_this.Reset();
    js_object_dealloc((js_object *) self);
}
