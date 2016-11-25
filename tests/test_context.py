import pytest
from v8py import JavaScriptTerminated

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

