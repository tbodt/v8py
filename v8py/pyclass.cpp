#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "pyfunction.h"
#include "pyclass.h"
#include "context.h"

int add_to_template(PyObject *cls, PyObject *member_name, PyObject *member_value, Local<FunctionTemplate> templ);

PyTypeObject py_class_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};

int py_class_type_init() {
    py_class_type.tp_name = "v8py.Class";
    py_class_type.tp_basicsize = sizeof(py_class);
    py_class_type.tp_flags = Py_TPFLAGS_DEFAULT;
    py_class_type.tp_doc = "";
    return PyType_Ready(&py_class_type);
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
        // I have a funny feeling this will be useful someday.
        /* printf("old class computed mro: "); */
        /* PyObject_Print(mro, stdout, 0); */
        /* printf("\n"); */
        Py_DECREF(mro_list);
    } else {
        mro = ((PyTypeObject *) cls)->tp_mro;
        assert(PyTuple_GetItem(mro, PyTuple_Size(mro) - 1) == (PyObject *) &PyBaseObject_Type);
        mro = PyTuple_GetSlice(mro, 0, PyTuple_Size(mro) - 1);
    }
    PyObject *templ = py_class_for_mro(mro);
    Py_DECREF(mro);
    return templ;
}

int compute_old_class_mro(PyObject *cls, PyObject *mro_list) {
#if PY_MAJOR_VERSION >= 3
    assert(0);
#else
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
#endif
}

PyObject *py_class_new(PyObject *mro) {
    IN_V8;

    // The convert functions require a context for function conversion, but we
    // don't have a context. So we use an empty context and special-case
    // functions.
    Local<Context> no_ctx;

    PyObject *cls = PyTuple_GetItem(mro, 0);
    assert(cls != (PyObject *) &PyBaseObject_Type);

    py_class *self = (py_class *) py_class_type.tp_alloc(&py_class_type, 0);
    PyErr_PROPAGATE(self);

    Local<External> js_self = External::New(isolate, self);
    ConstructorBehavior construct_allowed = ConstructorBehavior::kAllow;
    // Sadly, this can't work because V8 allocates objects constructed from a
    // template with kThrow with 0 internal fields, regardless of how many are
    // specified on the InstanceTemplate. This is a bug apparently. Instead, I
    // throw an exception in the construct callback.
    // if (PyObject_HasAttrString(cls, "__v8py_unconstructable__")) {
        // construct_allowed = ConstructorBehavior::kThrow;
    // }
    Local<FunctionTemplate> templ = FunctionTemplate::New(isolate, py_class_construct_callback, js_self,
            Local<Signature>(), 0, construct_allowed);

    PyObject *dict;
    if (PyClass_Check(cls)) {
        // old style
        dict = PyObject_GetAttr(cls, __dict__);
    } else {
        // new style
        // Sorry, but I have to do this to use PyDict_Next. At least the layout
        // is the same on 2 and 3.
        struct dictproxy {
            PyObject_HEAD
            PyObject *dict;
        };
        struct dictproxy *proxy = (struct dictproxy *) PyObject_GenericGetAttr(cls, __dict__);
        dict = proxy->dict;
        Py_INCREF(dict);
        Py_DECREF(proxy);
    }
    PyErr_PROPAGATE(dict);
    PyObject *member_name, *member_value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &member_name, &member_value)) {
        Py_INCREF(member_name);
        Py_INCREF(member_value);
        if (add_to_template(cls, member_name, member_value, templ) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }
    Py_DECREF(dict);

    templ->InstanceTemplate()->SetInternalFieldCount(OBJECT_INTERNAL_FIELDS);
    // if the class defines __getitem__ and keys(), it's a mapping.
    // if __setitem__ is implemented, the properties are writable.
    // if __delitem__ is implemented, the properties are configurable.
    if (PyObject_HasAttrString(cls, "__getitem__") &&
            PyObject_HasAttrString(cls, "keys")) {
        NamedPropertyHandlerConfiguration callbacks;
        callbacks.getter = named_getter;
        callbacks.enumerator = named_enumerator;
        callbacks.query = named_query;
        if (PyObject_HasAttrString(cls, "__setitem__")) {
            callbacks.setter = named_setter;
        }
        if (PyObject_HasAttrString(cls, "__delitem__")) {
            callbacks.deleter = named_deleter;
        }
        templ->InstanceTemplate()->SetHandler(callbacks);
    }
    // if __getitem__ and __len__ are defined, it's a sequence.
    if (PyObject_HasAttrString(cls, "__getitem__") &&
            PyObject_HasAttrString(cls, "__len__")) {
        IndexedPropertyHandlerConfiguration callbacks;
        callbacks.getter = indexed_getter;
        callbacks.enumerator = indexed_enumerator;
        callbacks.query = indexed_query;
        if (PyObject_HasAttrString(cls, "__setitem__")) {
            callbacks.setter = indexed_setter;
        }
        if (PyObject_HasAttrString(cls, "__delitem__")) {
            callbacks.deleter = indexed_deleter;
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
    // if 1 left and it's object, don't inherit
    // if 2 left and it's Exception -> BaseException, set magic internal field on prototype
    // if 1 left and it's BaseException don't worry about it (can only happen if directly converting instance of BaseException)
    if (PyTuple_Size(mro_rest) == 2 && 
            PyTuple_GetItem(mro_rest, 0) == PyExc_Exception &&
            PyTuple_GetItem(mro_rest, 1) == PyExc_BaseException) {
        // if the superclass is Exception, ignore that fact and make the superclass Error
        // we do this by putting a magic internal field count on the prototype which py_class_init_js_object detects
        // and sets the prototype's prototype to Error.prototype, which it finds in an embedder data slot on the context
        // it has to be a magic count instead of a field because you can't set an internal field on a template
        // only an object
        templ->PrototypeTemplate()->Set(JSTR("__proto__"), I_CAN_HAZ_ERROR_PROTOTYPE);
    } else if (PyTuple_Size(mro_rest) > 0 && 
            !(PyTuple_Size(mro_rest) == 1 && PyTuple_GetItem(mro_rest, 0) == (PyObject *) &PyBaseObject_Type)) {
        py_class *superclass_templ = (py_class *) py_class_for_mro(mro_rest);
        templ->Inherit(superclass_templ->templ->Get(isolate));
    }

    return (PyObject *) self;
}

// 0 on success, -1 on failure
int add_to_template(PyObject *cls, PyObject *member_name, PyObject *member_value, Local<FunctionTemplate> templ) {
    HandleScope hs(isolate);
    Local<Context> no_ctx;
    Local<Signature> sig = Signature::New(isolate, templ);

    // skip names that start with _ or are marked __v8py_hidden__
    if (PyString_StartsWithString(member_name, "_") ||
            PyObject_HasAttrString(member_value, "__v8py_hidden__")) {
        return 0;
    }
    Local<Name> js_name = js_from_py(member_name, no_ctx).As<Name>();
    Local<Data> js_value;

    if (PyFunction_Check(member_value)) {
        // if it's an unbound method, create a method
        // the member value is supposed to be not increfed because the class should hold a reference to it
        Local<External> js_method = External::New(isolate, member_value);
        js_value = FunctionTemplate::New(isolate, py_class_method_callback, js_method, sig);
    } else {
        if (PyObject_TypeCheck(member_value, &PyStaticMethod_Type) ||
                PyObject_TypeCheck(member_value, &PyClassMethod_Type)) {
            // if it's a staticmethod or classmethod, make a function from the callable within
            PyObject *callable = Py_TYPE(member_value)->tp_descr_get(member_value, NULL, (PyObject *) Py_TYPE(member_value));
            py_function *function = (py_function *) py_function_to_template(callable);
            Py_DECREF(callable);
            if (function == NULL) {
                return -1;
            }
            js_value = function->js_template->Get(isolate);
        } else if (PyObject_HasAttrString(member_value, "__get__") && PyObject_HasAttrString(member_value, "__set__")) {
            // if it's a descriptor, make an accessor
            templ->InstanceTemplate()->SetAccessor(js_name, py_class_property_getter, py_class_property_setter, 
                    js_name, DEFAULT, DontDelete);
        } else {
            // otherwise just convert
            js_value = js_from_py(member_value, no_ctx);
        }
        if (!js_value.IsEmpty()) {
            templ->Set(js_name, js_value);
        }
    }

    if (!js_value.IsEmpty()) {
        templ->PrototypeTemplate()->Set(js_name, js_value);
    }

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
    assert(js_object->GetInternalField(0) == IZ_DAT_OBJECT);
    PyObject *py_object = (PyObject *) js_object->GetInternalField(1).As<External>()->Value();

    // the entire purpose of this weak callback
    Py_DECREF(py_object);

    info.GetParameter()->Reset();
    delete info.GetParameter();
}

void py_class_init_js_object(Local<Object> js_object, PyObject *py_object, Local<Context> context) {
    js_object->SetInternalField(0, IZ_DAT_OBJECT);
    js_object->SetInternalField(1, External::New(isolate, py_object));

    // find out if the object is supposed to inherit from Error
    // the information is in an internal field on the last prototype
    Local<Value> last_proto = js_object;
    Local<Object> last_proto_object;
    while (!last_proto->StrictEquals(context->GetEmbedderData(OBJECT_PROTOTYPE_SLOT))) {
        last_proto_object = last_proto.As<Object>();
        last_proto = last_proto_object->GetPrototype();
    }
    // last_proto is guaranteed to have a value because the loop is guaranteed
    // to run at least once because last_proto_chain is initialized to js_object
    assert(!last_proto.IsEmpty());
    if (last_proto_object->Get(context, JSTR("__proto__")).ToLocalChecked()->StrictEquals(I_CAN_HAZ_ERROR_PROTOTYPE)) {
        last_proto_object->Delete(context, JSTR("__proto__")).FromJust();
        last_proto_object->SetPrototype(context->GetEmbedderData(ERROR_PROTOTYPE_SLOT));
    }

    Persistent<Object> *obj_handle = new Persistent<Object>(isolate, js_object);
    obj_handle->SetWeak(obj_handle, py_class_object_weak_callback, WeakCallbackType::kFinalizer);
}

Local<Object> py_class_create_js_object(py_class *self, PyObject *py_object, Local<Context> context) {
    EscapableHandleScope hs(isolate);
    Local<Object> js_object = self->templ->Get(isolate)->InstanceTemplate()->NewInstance(context).ToLocalChecked();
    Py_INCREF(py_object);
    py_class_init_js_object(js_object, py_object, context);
    return hs.Escape(js_object);
}

