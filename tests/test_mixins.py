import sys
import pytest

if sys.version_info.major < 3:
    params = ['new', 'old']
else:
    params = ['new']
@pytest.fixture(params=['new', 'old'])
def Mixin(request):
    if request.param == 'new':
        class Mixin(object):
            def mixin_method(self):
                return 'mixin method'
    else:
        class Mixin:
            def mixin_method(self):
                return 'mixin method'
    return Mixin

@pytest.fixture(params=['new', 'old'])
def Class(request, Mixin):
    if request.param == 'new':
        class Superclass(object):
            def supermethod(self):
                return 'supermethod'
    else:
        class Superclass:
            def supermethod(self):
                return 'supermethod'
    class Class(Mixin, Superclass):
        def method(self):
            return 'method'
    return Class

@pytest.fixture
def context(context, Class, Mixin):
    context.expose(Class)
    context.expose(Mixin)
    context.expose(Class.__bases__[-1])
    context.eval('thing = new Class()')
    return context

def test_mixin(context):
    assert context.eval('thing instanceof Superclass')
    assert not context.eval('thing instanceof Mixin')
    assert context.eval('thing.supermethod()') == 'supermethod'
    assert context.eval('thing.method()') == 'method'
    assert context.eval('thing.mixin_method()') == 'mixin method'
