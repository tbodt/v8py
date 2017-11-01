import pytest
import time

from v8py import JavaScriptTerminated, current_context, new

def test_glob(context):
    context.eval('foo = "bar"')
    assert context.glob.foo == 'bar'

def test_getattr(context):
    context.foo = 'bar'
    assert context.foo == 'bar'
    assert context.glob.foo == 'bar'
    assert context.eval('foo') == 'bar'

def test_getitem(context):
    context['foo'] = 'bar'
    assert context['foo'] == 'bar'
    assert context.glob['foo'] == 'bar'
    assert context.eval('foo') == 'bar'

def test_timeout(context):
    with pytest.raises(JavaScriptTerminated):
        context.eval('for(;;) {}', timeout=0.1)

def test_timeout_property(context_with_timeout):
    assert context_with_timeout.timeout == 0.1

    start = time.time()
    with pytest.raises(JavaScriptTerminated):
        context_with_timeout.eval('for(;;) {}')
    diff = time.time() - start
    assert diff >= 0.1 and diff < 0.2

    context_with_timeout.timeout = 0.25
    assert context_with_timeout.timeout == 0.25

    start = time.time()
    with pytest.raises(JavaScriptTerminated):
        context_with_timeout.eval('for(;;) {}')
    diff = time.time() - start
    assert diff >= 0.25 and diff < 0.3

def test_timeout_cotext_level(context_with_timeout):
    with pytest.raises(JavaScriptTerminated):
        context_with_timeout.eval('for(;;) {}')

def test_timeout_new(context_with_timeout):
    context_with_timeout.eval('function Freeze() { while(true); }')
    with pytest.raises(JavaScriptTerminated):
        new(context_with_timeout.glob.Freeze)

def test_timeout_call(context_with_timeout):
    context_with_timeout.eval('function freeze() { while(true); }')
    with pytest.raises(JavaScriptTerminated):
        context_with_timeout.glob.freeze()

def test_timeout_proxy(context_with_timeout):
    context_with_timeout.eval("""
        user = {};
        user.testA = 0;
        user.testC = 10;
        
        proxy = new Proxy(user, {
          get(target, prop) {
            if (prop == "testA") while(true);
          },
          set(target, prop, value) {
            if (prop == "testB") while(true);
            return false;
          },
          deleteProperty(target, phrase) {
            if (phrase == "testC") while(true);
            return false;
          }
        });
    """)

    proxy = context_with_timeout.glob.proxy

    with pytest.raises(JavaScriptTerminated):
        testA = proxy.testA

    with pytest.raises(JavaScriptTerminated):
        proxy.testB = 5

    with pytest.raises(JavaScriptTerminated):
        del proxy.testC

def test_expose(context):
    def f(): return 'f'
    def g(): return 'g'
    context.expose(f, g, h=f)
    assert context.eval('f()') == 'f'
    assert context.eval('g()') == 'g'
    assert context.eval('h()') == 'f'

def f(): pass

def test_expose_module(context):
    import test_context
    context.expose_module(test_context)
    assert context.eval('f()') is None

def test_current_context(context):
    assert current_context() is None
    def f():
        assert current_context() is context
    context.expose(f)
    context.eval('f()')
