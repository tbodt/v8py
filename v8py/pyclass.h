#ifndef CLASS_TEMPLATE_H
#define CLASS_TEMPLATE_H

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
void py_class_init_js_object(Local<Object> js_object, PyObject *py_object);

#define OBJ_MAGIC ((void *) 0xDaC1a550)

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info);
void py_class_method_callback(const FunctionCallbackInfo<Value> &info);

// Handlers
void py_class_getter_callback(Local<Name> name, const PropertyCallbackInfo<Value> &info);
void py_class_setter_callback(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value> &info);
void py_class_query_callback(Local<Name> name, const PropertyCallbackInfo<Integer> &info);
void py_class_deleter_callback(Local<Name> name, const PropertyCallbackInfo<Boolean> &info);
void py_class_enumerator_callback(const PropertyCallbackInfo<Array> &info);

#endif
