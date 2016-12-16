#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "convert.h"
#include "pyfunction.h"
#include "pyclass.h"
#include "jsobject.h"
#include "pydictionary.h"

PyObject *py_from_js(Local<Value> value, Local<Context> context) {
    IN_V8;

    if (value->IsSymbol()) {
        value = value.As<Symbol>()->Name();
    }

    if (value->IsNull()) {
        Py_INCREF(null_object);
        return null_object;
    }
    if (value->IsUndefined()) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (value->IsBoolean()) {
        Local<Boolean> bool_value = value.As<Boolean>();
        if (bool_value->Value()) {
            Py_INCREF(Py_True);
            return Py_True;
        } else {
            Py_INCREF(Py_False);
            return Py_False;
        }
    }

    if (value->IsArray()) {
        Local<Array> array = value.As<Array>();
        PyObject *list = PyList_New(array->Length());
        for (unsigned i = 0; i < array->Length(); i++) {
            PyObject *obj = py_from_js(array->Get(context, i).ToLocalChecked(), context);
            if (obj == NULL) {
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(list, i, obj);
        }
        return list;
    }
    
    if (value->IsObject()) {
        Local<Object> obj_value = value.As<Object>();
        if (obj_value->InternalFieldCount() == OBJECT_INTERNAL_FIELDS ||
                obj_value->InternalFieldCount() == DICT_INTERNAL_FIELDS) {
            Local<Value> magic = obj_value->GetInternalField(0);
            if (magic == IZ_DAT_OBJECT || magic == IZ_DAT_DICTINARY) {
                PyObject *object = (PyObject *) obj_value->GetInternalField(1).As<External>()->Value();
                Py_INCREF(object);
                return object;
            }
        }
        return (PyObject *) js_object_new(obj_value, context);
    }

    if (value->IsString()) {
        Local<String> str_value = value.As<String>();
        size_t bufsize = str_value->Length() * sizeof(uint16_t);
        uint16_t *buf = (uint16_t *) malloc(bufsize);
        PyErr_PROPAGATE(buf);
        str_value->Write(buf, 0, bufsize, String::WriteOptions::NO_NULL_TERMINATION);
        PyObject *py_value = PyUnicode_DecodeUTF16((const char *) buf, bufsize, NULL, NULL);
        free(buf);
        return py_value;
    }
    if (value->IsUint32() || value->IsInt32()) {
        return PyLong_FromLongLong((PY_LONG_LONG) value.As<Integer>()->Value());
    }
    if (value->IsNumber()) {
        return PyFloat_FromDouble(value.As<Number>()->Value());
    }

    // i'm not quite ready to turn this into an assert quite yet
    // i'll do that when I've special-cased every primitive type
    printf("cannot convert\n");
    Py_RETURN_NONE;
}

Local<Value> js_from_py(PyObject *value, Local<Context> context) {
    ESCAPING_IN_V8;

    if (value == Py_False) {
        return hs.Escape(False(isolate));
    }
    if (value == Py_True) {
        return hs.Escape(True(isolate));
    }
    if (value == Py_None) {
        return hs.Escape(Undefined(isolate));
    }
    if (value == null_object) {
        return hs.Escape(Null(isolate));
    }

#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(value)) {
        Py_ssize_t len;
        char *str = PyUnicode_AsUTF8AndSize(value, &len);
        Local<String> js_value = String::NewFromUtf8(isolate, str, NewStringType::kNormal, len).ToLocalChecked();
        return hs.Escape(js_value);
    }
    if (PyString_Check(value)) {
        char *str;
        Py_ssize_t len;
        PyBytes_AsStringAndSize(value, &str, &len);
        Local<ArrayBuffer> js_value = ArrayBuffer::New(isolate, len);
        memcpy(str, js_value->GetContents().Data(), len);
        return hs.Escape(js_value);
    }
#else
    if (PyUnicode_Check(value)) {
        PyObject *value_encoded = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(value), PyUnicode_GET_SIZE(value), NULL);
        Local<String> js_value = String::NewFromUtf8(isolate, PyString_AS_STRING(value_encoded), NewStringType::kNormal, PyString_GET_SIZE(value_encoded)).ToLocalChecked();
        Py_DECREF(value_encoded);
        return hs.Escape(js_value);
    } 
    if (PyString_Check(value)) {
        Local<String> js_value = String::NewFromUtf8(isolate, PyString_AS_STRING(value), NewStringType::kNormal, PyString_GET_SIZE(value)).ToLocalChecked();
        return hs.Escape(js_value);
    }
#endif

    if (PyNumber_Check(value) && !PyInstance_Check(value)) {
        Local<Number> js_value;
        if (PyFloat_Check(value)) {
            js_value = Number::New(isolate, PyFloat_AS_DOUBLE(value));
        } else if (PyLong_Check(value)) {
            js_value = Integer::New(isolate, PyLong_AsLong(value));
#if PY_MAJOR_VERSION < 3
        } else if (PyInt_Check(value)) {
            js_value = Integer::New(isolate, PyInt_AS_LONG(value));
#endif
        } else {
            // TODO make this work right
            printf("what the hell kind of number is this?!");
            return hs.Escape(Undefined(isolate));
        }
        return hs.Escape(js_value);
    }

    if (PyDict_Check(value)) {
        return hs.Escape(py_dictionary_get_proxy(value, context));
    }

    if (PyList_Check(value) || PyTuple_Check(value)) {
        int length = PySequence_Length(value);
        Local<Array> array = Array::New(isolate, length);
        for (int i = 0; i < length; i++) {
            PyObject *item = PySequence_ITEM(value, i);
            bool set_worked = array->Set(context, i, js_from_py(item, context)).FromJust();
            assert(set_worked);
            Py_DECREF(item);
        }
        return hs.Escape(array);
    }

    if (PyFunction_Check(value)) {
        py_function *templ = (py_function *) py_function_to_template(value);
        return hs.Escape(py_template_to_function(templ, context));
    }

    if (PyType_Check(value) || PyClass_Check(value)) {
        py_class *templ = (py_class *) py_class_to_template(value);
        return hs.Escape(py_class_get_constructor(templ, context));
    }

    if (PyObject_TypeCheck(value, &js_object_type)) {
        js_object *py_value = (js_object *) value;
        return hs.Escape(py_value->object.Get(isolate));
    }

    // it's an arbitrary object
    PyObject *type;
    if (PyInstance_Check(value)) {
        type = PyObject_GetAttrString(value, "__class__");
    } else {
        type = (PyObject *) Py_TYPE(value);
        Py_INCREF(type);
    }
    py_class *templ = (py_class *) py_class_to_template(type);
    Py_DECREF(type);
    return hs.Escape(py_class_create_js_object(templ, value, context));
}

PyObject *pys_from_jss(const FunctionCallbackInfo<Value> &js_args, Local<Context> context) {
    PyObject *py_args = PyTuple_New(js_args.Length());
    PyErr_PROPAGATE(py_args);
    for (int i = 0; i < js_args.Length(); i++) {
        PyObject *arg = py_from_js(js_args[i], context);
        if (arg == NULL) {
            Py_DECREF(py_args);
            return NULL;
        }
        PyTuple_SET_ITEM(py_args, i, arg);
    }
    return py_args;
}

// js_args is an out parameter, expected to contain enough space
void jss_from_pys(PyObject *py_args, Local<Value> *js_args, Local<Context> context) {
    for (int i = 0; i < PyTuple_GET_SIZE(py_args); i++) {
        js_args[i] = js_from_py(PyTuple_GET_ITEM(py_args, i), context);
    }
}

