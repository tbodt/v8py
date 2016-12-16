#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyBaseExceptionObject base;
    Persistent<Value> exception;
    Persistent<Message> message;
} js_exception;
extern PyTypeObject js_exception_type;
int js_exception_type_init();
extern PyTypeObject js_terminated_type;
int js_terminated_type_init();

PyObject *js_exception_new(Local<Value> exception, Local<Message> message);
void js_exception_dealloc(js_exception *self);
PyObject *js_exception_str(js_exception *self);

void py_throw_js(Local<Value> js_exc, Local<Message> js_message);
#define JS_TRY TryCatch tc(isolate);
#define PY_PROPAGATE_JS_RET(retval) \
    if (tc.HasCaught()) { \
        if (tc.CanContinue()) { \
            py_throw_js(tc.Exception(), tc.Message()); \
        } else { \
            PyErr_SetNone((PyObject *) &js_terminated_type); \
        } \
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
#define JS_PROPAGATE_PY_(value) \
    if (value < 0) { \
        js_throw_py(); \
        return; \
    }

#endif
