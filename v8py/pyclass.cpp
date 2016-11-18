#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "pyfunction.h"
#include "pyclass.h"

PyTypeObject py_class_type = {
    PyObject_HEAD_INIT(NULL)
};

int py_class_type_init() {
    py_class_type.tp_name = "v8py.Class";
    py_class_type.tp_basicsize = sizeof(py_class);
    py_class_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_class_type.tp_doc = "";
    return PyType_Ready(&py_class_type);
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
        PyErr_PROPAGATE(template_dict);
    }

    PyObject *templ = PyDict_GetItem(template_dict, cls);
    if (templ != NULL) {
        // PyDict_GetItem returns a borrowed reference
        Py_INCREF(templ);
        return templ;
    }

    templ = py_class_new(cls);
    PyDict_SetItem(template_dict, cls, templ);
    Py_DECREF(templ);
    return templ;
}

PyObject *py_class_new(PyObject *cls) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);

    // The convert functions require a context for function conversion, but we
    // don't have a context. So we use a null context and special-case
    // functions.
    Local<Context> no_ctx;

    py_class *self = (py_class *) py_class_type.tp_alloc(&py_class_type, 0);
    PyErr_PROPAGATE(self);
    // See template.cpp for why I'm doing this.
    Py_INCREF(self);

    Py_INCREF(cls);
    self->cls = cls;
    self->cls_name = PyObject_GetAttrString(cls, "__name__");
    if (self->cls_name == NULL) {
        Py_DECREF(self->cls);
    }

    Local<External> js_self = External::New(isolate, self);
    Local<FunctionTemplate> templ = FunctionTemplate::New(isolate, py_class_construct_callback, js_self);
    Local<ObjectTemplate> prototype_templ = templ->PrototypeTemplate();
    Local<Signature> signature = Signature::New(isolate, templ);

    PyObject *attributes = PyObject_Dir(cls);
    if (attributes == NULL) {
        goto attributes_failed;
    }
    for (int i = 0; i < PySequence_Length(attributes); i++) {
        PyObject *attrib_name = PySequence_ITEM(attributes, i);
        if (attrib_name == NULL) {
            goto attrib_failed;
        }
        // skip names with too many underscores
        if (py_bool(PyObject_CallMethod(attrib_name, "startswith", "s", "__")) &&
                py_bool(PyObject_CallMethod(attrib_name, "endswith", "s", "__"))) {
            Py_DECREF(attrib_name);
            continue;
        }
        PyObject *attrib_value = PyObject_GetAttr(cls, attrib_name);
        if (attrib_value == NULL) {
            Py_DECREF(attrib_value);
            goto attrib_failed;
        }
        Local<Name> js_name = js_from_py(attrib_name, no_ctx).As<Name>();
        Local<Data> js_value;

        if (PyMethod_Check(attrib_value) && PyMethod_GET_SELF(attrib_value) == NULL) {
            // if it's an unbound method, create a method
            js_value = FunctionTemplate::New(isolate, py_class_method_callback, js_name, signature);
        } else {
            if (PyCallable_Check(attrib_value)) {
                // if it's some kind of callable, create a function
                py_function *function = (py_function *) py_function_to_template(attrib_value);
                if (function == NULL) {
                    Py_DECREF(attrib_name);
                    Py_DECREF(attrib_value);
                    goto attrib_failed;
                }
                js_value = function->js_template->Get(isolate);
            } else {
                // otherwise just convert
                js_value = js_from_py(attrib_value, no_ctx);
            }
            templ->Set(js_name, js_value);
        }

        prototype_templ->Set(js_name, js_value);
        Py_DECREF(attrib_name);
        Py_DECREF(attrib_value);
    }

    templ->SetClassName(js_from_py(self->cls_name, no_ctx).As<String>());
    // first one is magic pointer
    // second one is actual object
    templ->InstanceTemplate()->SetInternalFieldCount(2);

    // if the class defines __getitem__ and keys(), it's a mapping.
    // if __setitem__ is implemented, the properties are writable.
    // if __delitem__ is implemented, the properties are configurable.
    if (PyObject_HasAttrString(cls, "__getitem__") &&
            PyObject_HasAttrString(cls, "keys")) {
        NamedPropertyHandlerConfiguration callbacks;
        callbacks.getter = py_class_getter_callback;
        callbacks.enumerator = py_class_enumerator_callback;
        callbacks.query = py_class_query_callback;
        if (PyObject_HasAttrString(cls, "__setitem__")) {
            callbacks.setter = py_class_setter_callback;
        }
        if (PyObject_HasAttrString(cls, "__delitem__")) {
            callbacks.deleter = py_class_deleter_callback;
        }
        templ->InstanceTemplate()->SetHandler(callbacks);
    }

    self->templ = new Persistent<FunctionTemplate>();
    self->templ->Reset(isolate, templ);

    Py_DECREF(attributes);
    return (PyObject *) self;

attrib_failed:
    Py_DECREF(attributes);
attributes_failed:
    Py_DECREF(self->cls_name);
    Py_DECREF(self->cls);
    Py_DECREF(self);
    return NULL;
}

Local<Function> py_class_get_constructor(py_class *self, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Function> function = self->templ->Get(isolate)->GetFunction(context).ToLocalChecked();
    function->SetName(js_from_py(self->cls_name, context).As<String>());
    return hs.Escape(function);
}

void py_class_object_weak_callback(const WeakCallbackInfo<Persistent<Object>> &info) {
    HandleScope hs(isolate);
    Local<Object> js_object = info.GetParameter()->Get(isolate);
    assert(js_object->GetAlignedPointerFromInternalField(0) == OBJ_MAGIC);
    PyObject *py_object = (PyObject *) js_object->GetInternalField(1).As<External>()->Value();

    // the entire purpose of this weak callback
    Py_DECREF(py_object);

    info.GetParameter()->Reset();
    delete info.GetParameter();
}

void py_class_init_js_object(Local<Object> js_object, PyObject *py_object) {
    js_object->SetAlignedPointerInInternalField(0, OBJ_MAGIC);
    js_object->SetInternalField(1, External::New(isolate, py_object));

    Persistent<Object> *obj_handle = new Persistent<Object>(isolate, js_object);
    obj_handle->SetWeak(obj_handle, py_class_object_weak_callback, WeakCallbackType::kFinalizer);
}

Local<Object> py_class_create_js_object(py_class *self, PyObject *py_object, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Object> js_object = self->templ->Get(isolate)->InstanceTemplate()->NewInstance(context).ToLocalChecked();
    Py_INCREF(py_object);
    py_class_init_js_object(js_object, py_object);
    return hs.Escape(js_object);
}

