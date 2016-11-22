#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    // copied from PyBaseExceptionObject {
    PyObject *dict;
    PyObject *py_args;
    PyObject *py_message;
    // }
    Persistent<Value> exception;
    Persistent<Message> message;
} js_exception;
extern PyTypeObject js_exception_type;
int js_exception_type_init();

PyObject *js_exception_new(Local<Value> exception, Local<Message> message);
void js_exception_dealloc(js_exception *self);

PyObject *js_exception_str(js_exception *self);

void py_throw_js(Local<Value> js_exc, Local<Message> js_message);
#define JS_TRY TryCatch tc(isolate);
#define PY_PROPAGATE_JS_RET(retval) \
    if (tc.HasCaught()) { \
        py_throw_js(tc.Exception(), tc.Message()); \
        return retval; \
    }
#define PY_PROPAGATE_JS PY_PROPAGATE_JS_RET(NULL)
#define PY_PROPAGATE_JS_ PY_PROPAGATE_JS_RET(-1)

void js_throw_py();
#define JS_PROPAGATE_PY(value) \
    if (value == NULL) { \
        js_throw_py(); \
        return; \
    }

