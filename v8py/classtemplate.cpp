#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "template.h"
#include "classtemplate.h"

PyTypeObject py_class_template_type = {
    PyObject_HEAD_INIT(NULL)
};

int py_class_template_type_init() {
    py_class_template_type.tp_name = "v8py.ClassTemplate";
    py_class_template_type.tp_basicsize = sizeof(py_class_template);
    py_class_template_type.tp_dealloc = (destructor) py_class_template_dealloc;
    py_class_template_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_class_template_type.tp_doc = "";
    py_class_template_type.tp_new = py_fake_new;
    return PyType_Ready(&py_class_template_type);
}

int py_bool(PyObject *obj) {
    assert(PyBool_Check(obj));
    int retval = (obj == Py_True);
    Py_DECREF(obj);
    return retval;
}

static PyObject *template_dict = NULL;

PyObject *py_class_to_template(PyObject *cls) {
    if (template_dict == NULL) {
        template_dict = PyDict_New();
        if (template_dict == NULL) {
            return NULL;
        }
    }

    PyObject *templ = PyDict_GetItem(template_dict, cls);
    if (templ != NULL) {
        Py_INCREF(templ);
        return templ;
    }

    templ = py_class_template_new(cls);
    PyDict_SetItem(template_dict, cls, templ);
    return templ;
}

Persistent<Object> object_magic;

PyObject *py_class_template_new(PyObject *cls) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);

    // The convert functions require a context for function conversion, but we
    // don't have a context. So we use a null context and special-case
    // functions.
    Local<Context> no_ctx;

    // This magic object goes in the first internal field so any other object
    // with internal fields doesn't look like one of ours.
    if (magic.IsEmpty()) {
        magic.Reset(Object::New(isolate));
    }

    py_class_template *self = (py_class_template *) py_class_template_type.tp_alloc(&py_class_template_type, 0);
    if (self == NULL) {
        return NULL;
    }
    // See template.cpp for why I'm doing this.
    Py_INCREF(self);

    self->cls = cls;
    self->cls_name = PyObject_GetAttrString(cls, "__name__");

    Local<External> js_self = External::New(isolate, self);
    Local<FunctionTemplate> templ = FunctionTemplate::New(isolate, py_class_construct_callback, js_self);
    Local<ObjectTemplate> prototype_templ = templ->PrototypeTemplate();
    Local<Signature> signature = Signature::New(isolate, templ);

    PyObject *attributes = PyObject_Dir(cls);
    for (int i = 0; i < PySequence_Length(attributes); i++) {
        PyObject *attrib_name = PySequence_ITEM(attributes, i);
        if (py_bool(PyObject_CallMethod(attrib_name, "startswith", "s", "__")) &&
                py_bool(PyObject_CallMethod(attrib_name, "endswith", "s", "__"))) {
            Py_DECREF(attrib_name);
            continue;
        }
        PyObject *attrib_value = PyObject_GetAttr(cls, attrib_name);
        Local<Name> js_name = js_from_py(attrib_name, no_ctx).As<Name>();
        Local<Data> js_value;

        if (PyMethod_Check(attrib_value)) {
            js_value = FunctionTemplate::New(isolate, py_class_method_callback, js_name, signature);
        } else if (PyFunction_Check(attrib_value)) {
            js_value = ((py_template *) py_function_to_template(attrib_value))->js_template->Get(isolate);
        } else {
            js_value = js_from_py(attrib_value, no_ctx);
        }

        prototype_templ->Set(js_name, js_value);
        Py_DECREF(attrib_name);
        Py_DECREF(attrib_value);
    }
    Py_DECREF(attributes);

    templ->SetClassName(js_from_py(self->cls_name, no_ctx).As<String>());
    // first one is magic object
    // second one is actual pointer
    templ->InstanceTemplate()->SetInternalFieldCount(2);

    self->templ = new Persistent<FunctionTemplate>();
    self->templ->Reset(isolate, templ);

    return (PyObject *) self;
}

Local<Function> py_class_get_constructor(py_class_template *self, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Function> function = self->templ->Get(isolate)->GetFunction(context).ToLocalChecked();
    function->SetName(js_from_py(self->cls_name, context).As<String>());
    return hs.Escape(function);
}

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    py_class_template *self = (py_class_template *) info.Data().As<External>()->Value();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> js_new_object = info.This();

    if (js_new_object->InternalFieldCount() != 1) {
        // TODO: throw a JS exception
        // "this is supposed to be called like a constructor" or something to that effect
        // either that or self->templ->GetFunction(context)->CallAsConstructor()
        return;
    }

    PyObject *new_object = PyObject_Call(self->cls, pys_from_jss(info, context), NULL);
    js_new_object->SetInternalField(0, magic);
    js_new_object->SetInternalField(1, External::New(isolate, new_object));
}

void py_class_method_callback(const FunctionCallbackInfo<Value> &info) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> js_self = info.This();
    PyObject *self = (PyObject *) js_self->GetInternalField(0).As<External>()->Value();
    PyObject *args = pys_from_jss(info, context);

    PyObject *method_name = py_from_js(info.Data(), context);
    PyObject *method = PyObject_GetAttr(self, method_name);
    PyObject *retval = PyObject_Call(method, args, NULL);

    info.GetReturnValue().Set(js_from_py(retval, context));

    Py_DECREF(method_name);
    Py_DECREF(method);
    Py_DECREF(args);
    Py_DECREF(retval);
}

void py_class_template_dealloc(py_class_template *self) {
    printf("this should never happen\n");
    assert(0);
}

