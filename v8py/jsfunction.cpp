#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "jsobject.h"
#include "convert.h"

PyTypeObject js_function_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int js_function_type_init() {
    js_function_type.tp_name = "v8py.BoundFunction";
    js_function_type.tp_basicsize = sizeof(js_function);
    js_function_type.tp_dealloc = (destructor) js_function_dealloc;
    js_function_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_function_type.tp_doc = "";
    js_function_type.tp_call = (ternaryfunc) js_function_call;
    js_function_type.tp_base = &js_object_type;
    return PyType_Ready(&js_function_type);
}

PyObject *js_function_call(js_function *self, PyObject *args, PyObject *kwargs) {
    IN_V8;
    IN_CONTEXT(self->context.Get(isolate));
    JS_TRY

    Local<Object> object = self->object.Get(isolate);
    Local<Value> js_this;
    if (self->js_this.IsEmpty()) {
        js_this = Undefined(isolate);
    } else {
        js_this = self->js_this.Get(isolate);
    }
    int argc = PyTuple_GET_SIZE(args);
    Local<Value> *argv = new Local<Value>[argc];
    jss_from_pys(args, argv, context);
    MaybeLocal<Value> result = object->CallAsFunction(context, js_this, argc, argv);
    delete[] argv;
    PY_PROPAGATE_JS;
    return py_from_js(result.ToLocalChecked(), context);
}

void js_function_dealloc(js_function *self) {
    self->js_this.Reset();
    js_object_dealloc((js_object *) self);
}
