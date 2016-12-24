#ifndef CLASS_TEMPLATE_H
#define CLASS_TEMPLATE_H

#include <Python.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    PyObject *cls;
    PyObject *cls_name;
    Persistent<FunctionTemplate> *templ;
} py_class;
int py_class_type_init();
extern PyTypeObject py_class_type;

void py_class_dealloc(py_class *self);
PyObject *py_class_new(PyObject *cls);
PyObject *py_class_to_template(PyObject *cls);
Local<Function> py_class_get_constructor(py_class *self, Local<Context> context);
Local<Object> py_class_create_js_object(py_class *self, PyObject *py_object, Local<Context> context);
void py_class_init_js_object(Local<Object> js_object, PyObject *py_object, Local<Context> context);

// first one is magic pointer
// second one is actual object
// third is exception traceback (usually unused)
// fourth is exception type (also usually unused)
#define OBJECT_INTERNAL_FIELDS 4

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info);
void py_class_method_callback(const FunctionCallbackInfo<Value> &info);

// Handlers
void named_getter(Local<Name> name, const PropertyCallbackInfo<Value> &info);
void named_setter(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value> &info);
void named_query(Local<Name> name, const PropertyCallbackInfo<Integer> &info);
void named_deleter(Local<Name> name, const PropertyCallbackInfo<Boolean> &info);
void named_enumerator(const PropertyCallbackInfo<Array> &info);

void indexed_getter(uint32_t index, const PropertyCallbackInfo<Value> &info);
void indexed_setter(uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value> &info);
void indexed_query(uint32_t index, const PropertyCallbackInfo<Integer> &info);
void indexed_deleter(uint32_t index, const PropertyCallbackInfo<Boolean> &info);
void indexed_enumerator(const PropertyCallbackInfo<Array> &info);

void py_class_property_getter(Local<Name> js_name, const PropertyCallbackInfo<Value> &info);
void py_class_property_setter(Local<Name> js_name, Local<Value> js_value, const PropertyCallbackInfo<void> &info);

#endif
