#include <Python.h>
#include <pthread.h>
#include <v8.h>
#include <greenstack/greenstack.h>

#include "v8py.h"

using namespace v8;

// secret internal fields of the isolate that we need to access even though we aren't supposed to
namespace v8 {
    namespace base {
        class Thread {
            public:
                typedef int32_t LocalStorageKey;
        };
    }
    namespace internal {
        class Isolate {
            public:
                static base::Thread::LocalStorageKey isolate_key_;
                static base::Thread::LocalStorageKey per_isolate_thread_data_key_;
                static base::Thread::LocalStorageKey thread_id_key_;
        };
    }
}

static pthread_key_t isolate_key;
static pthread_key_t thread_data_key;
static pthread_key_t thread_id_key;

void grab_tls_keys() {
    isolate_key = internal::Isolate::isolate_key_;
    thread_data_key = internal::Isolate::per_isolate_thread_data_key_;
    thread_id_key = internal::Isolate::thread_id_key_;
}

void greenstack_actually_switch(void *data) {
        Isolate *isolate = (Isolate *) pthread_getspecific(isolate_key);
        void *thread_data = pthread_getspecific(thread_data_key);
        void *thread_id = pthread_getspecific(thread_id_key);

        PyGreenstack_CALL_SWITCH(data);

        pthread_setspecific(isolate_key, isolate);
        pthread_setspecific(thread_data_key, thread_data);
        pthread_setspecific(thread_id_key, thread_id);
}

void greenstack_switch_v8(void *data) {
    if (Locker::IsLocked(isolate)) {
        Unlocker unlocker(isolate);
        return greenstack_actually_switch(data);
    }
    return greenstack_actually_switch(data);
}

void greenstack_init_v8() {
    pthread_setspecific(isolate_key, NULL);
    pthread_setspecific(thread_data_key, NULL);
    pthread_setspecific(thread_id_key, NULL);
}

int greenstack_init() {
    PyGreenstack_Import();
    if (_PyGreenstack_API == NULL) {
        // No greenlets? No problems!
        return 0;
    }
    if (PyGreenstack_AddStateHandler(greenstack_switch_v8, greenstack_init_v8) < 0) {
        return -1;
    }
    grab_tls_keys();
    return 0;
}
