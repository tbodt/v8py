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
    py_class_type.tp_dealloc = (destructor) py_class_dealloc;
    py_class_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_class_type.tp_doc = "";
    py_class_type.tp_new = py_fake_new;
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
        if (template_dict == NULL) {
            return NULL;
        }
    }

    PyObject *templ = PyDict_GetItem(template_dict, cls);
    if (templ != NULL) {
        Py_INCREF(templ);
        return templ;
    }

    templ = py_class_new(cls);
    PyDict_SetItem(template_dict, cls, templ);
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
            js_value = ((py_function *) py_function_to_template(attrib_value))->js_template->Get(isolate);
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
    templ->InstanceTemplate()->SetHandler(NamedPropertyHandlerConfiguration(
                py_class_getter_callback,
                py_class_setter_callback,
                py_class_query_callback,
                py_class_deleter_callback,
                py_class_enumerator_callback));

    self->templ = new Persistent<FunctionTemplate>();
    self->templ->Reset(isolate, templ);

    return (PyObject *) self;
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

void py_class_dealloc(py_class *self) {
    printf("this should never happen\n");
    assert(0);
}

