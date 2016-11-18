#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "pyfunction.h"
#include "pyclass.h"

int add_to_template(PyObject *cls, PyObject *member_name, PyObject *member_value, Local<FunctionTemplate> templ, Local<Signature> signature);

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

PyObject *py_class_for_mro(PyObject *mro) {
    if (template_dict == NULL) {
        template_dict = PyDict_New();
        PyErr_PROPAGATE(template_dict);
    }

    PyObject *templ = PyDict_GetItem(template_dict, mro);
    if (templ != NULL) {
        // PyDict_GetItem returns a borrowed reference
        Py_INCREF(templ);
        return templ;
    }

    assert(PyTuple_Size(mro) > 0);
    templ = py_class_new(mro);
    PyErr_PROPAGATE(templ);
    if (PyDict_SetItem(template_dict, mro, templ) < 0) {
        return NULL;
    }
    return templ;
}

int compute_old_class_mro(PyObject *cls, PyObject *mro_list);

PyObject *py_class_to_template(PyObject *cls) {
    PyObject *mro;
    if (PyClass_Check(cls)) {
        PyObject *mro_list = PyList_New(0);
        PyErr_PROPAGATE(mro_list);
        if (compute_old_class_mro(cls, mro_list) < 0) {
            Py_DECREF(mro_list);
            return NULL;
        }
        mro = PySequence_Tuple(mro_list);
        /* printf("old class computed mro: "); */
        /* PyObject_Print(mro, stdout, 0); */
        /* printf("\n"); */
        Py_DECREF(mro_list);
    } else {
        mro = ((PyTypeObject *) cls)->tp_mro;
        Py_INCREF(mro);
    }
    PyObject *templ = py_class_for_mro(mro);
    Py_DECREF(mro);
    return templ;
}

int compute_old_class_mro(PyObject *cls, PyObject *mro_list) {
    if (PyList_Append(mro_list, cls) < 0) {
        return -1;
    }
    PyObject *bases = PyClass_GET_BASES(cls);
    for (int i = 0; i < PySequence_Length(bases); i++) {
        PyObject *base = PySequence_GetItem(bases, i);
        PyErr_PROPAGATE_(base);
        if (compute_old_class_mro(base, mro_list) < 0) {
            Py_DECREF(base);
            return -1;
        }
        Py_DECREF(base);
    }
    return 0;
}

PyObject *py_class_new(PyObject *mro) {
    Isolate::Scope is(isolate);
    HandleScope hs(isolate);

    // The convert functions require a context for function conversion, but we
    // don't have a context. So we use a null context and special-case
    // functions.
    Local<Context> no_ctx;

    PyObject *cls = PyTuple_GetItem(mro, 0);
    assert(cls != (PyObject *) &PyBaseObject_Type);

    py_class *self = (py_class *) py_class_type.tp_alloc(&py_class_type, 0);
    PyErr_PROPAGATE(self);

    Local<External> js_self = External::New(isolate, self);
    Local<FunctionTemplate> templ = FunctionTemplate::New(isolate, py_class_construct_callback, js_self);
    Local<Signature> signature = Signature::New(isolate, templ);

    PyObject *attributes = PyObject_Dir(cls);
    PyErr_PROPAGATE(attributes);
    for (int i = 0; i < PySequence_Length(attributes); i++) {
        PyObject *member_name = PySequence_GetItem(attributes, i);
        if (member_name == NULL) {
            Py_DECREF(attributes);
            return NULL;
        }
        PyObject *member_value = PyObject_GetAttr(cls, member_name);
        if (member_value == NULL) {
            Py_DECREF(member_name);
            Py_DECREF(attributes);
            return NULL;
        }
        int result = add_to_template(cls, member_name, member_value, templ, signature);
        Py_DECREF(member_name);
        Py_DECREF(member_value);
        if (result < 0) {
            Py_DECREF(attributes);
            return NULL;
        }
    }
    Py_DECREF(attributes);

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

    Py_INCREF(cls);
    self->cls = cls;
    self->cls_name = PyObject_GetAttrString(cls, "__name__");
    if (self->cls_name == NULL) {
        Py_DECREF(self->cls);
        return NULL;
    }
    templ->SetClassName(js_from_py(self->cls_name, no_ctx).As<String>());

    PyObject *mro_rest = PyTuple_GetSlice(mro, 1, PyTuple_Size(mro));
    if (PyTuple_Size(mro_rest) != 0 && PyTuple_GetItem(mro_rest, 0) != (PyObject *) &PyBaseObject_Type ) {
        py_class *superclass_templ = (py_class *) py_class_for_mro(mro_rest);
        templ->Inherit(superclass_templ->templ->Get(isolate));
    }

    return (PyObject *) self;
}

// 0 on success, -1 on failure
int add_to_template(PyObject *cls, PyObject *member_name, PyObject *member_value, Local<FunctionTemplate> templ, Local<Signature> signature) {
    HandleScope hs(isolate);
    Local<Context> no_ctx;

    // skip names with too many underscores
    if (py_bool(PyObject_CallMethod(member_name, "startswith", "s", "__")) &&
            py_bool(PyObject_CallMethod(member_name, "endswith", "s", "__"))) {
        return 0;
    }
    Local<Name> js_name = js_from_py(member_name, no_ctx).As<Name>();
    Local<Data> js_value;

    if (PyMethod_Check(member_value) && PyMethod_GET_SELF(member_value) == NULL) {
        // if it's an unbound method, create a method
        method_callback_info *callback_info = (method_callback_info *) malloc(sizeof(method_callback_info));
        // When are we going to free the callback info, you ask?
        // Answer: Never. This entire class can never be freed because FunctionTemplates never go away.
        Py_INCREF(cls);
        callback_info->cls = cls;
        Py_INCREF(member_name);
        callback_info->method_name = member_name;
        Local<External> js_method = External::New(isolate, callback_info);
        js_value = FunctionTemplate::New(isolate, py_class_method_callback, js_method, signature);
    } else {
        if (PyCallable_Check(member_value)) {
            // if it's some kind of callable, create a function
            py_function *function = (py_function *) py_function_to_template(member_value);
            if (function == NULL) {
                return -1;
            }
            js_value = function->js_template->Get(isolate);
        } else {
            // otherwise just convert
            js_value = js_from_py(member_value, no_ctx);
        }
        templ->Set(js_name, js_value);
    }

    templ->PrototypeTemplate()->Set(js_name, js_value);

    return 0;
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

