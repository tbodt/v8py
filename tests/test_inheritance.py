import pytest

@pytest.fixture(params=['new', 'old'])
def Class(request):
    if request.param == 'new':
        class Superclass(object):
            def supermethod(self):
                return 'supermethod'
            def method(self):
                return 'method in superclass'
        class Class(Superclass):
            def method(self):
                return 'method in subclass'
            def submethod(self):
                return 'submethod'
        return Class
    else:
        class Superclass:
            def supermethod(self):
                return 'supermethod'
            def method(self):
                return 'method in superclass'
        class Class(Superclass):
            def method(self):
                return 'method in subclass'
            def submethod(self):
                return 'submethod'
        return Class

@pytest.fixture
def context(context, Class):
    context.Class = Class
    context.Superclass = Class.__bases__[0]
    context.eval('subthing = new Class()')
    context.eval('superthing = new Superclass()')
    return context

def test_instanceof(context):
    assert context.eval('subthing instanceof Class')
    assert context.eval('subthing instanceof Superclass')
    assert not context.eval('superthing instanceof Class')

def test_things_are_functions(context):
    assert context.eval('Class.prototype.method instanceof Function')
    assert context.eval('Class.prototype.submethod instanceof Function')
    assert context.eval('Superclass.prototype.supermethod instanceof Function')
    assert context.eval('Superclass.prototype.method instanceof Function')

def test_sub_method_calls(context):
    assert context.eval('subthing.submethod()') == 'submethod'
    assert context.eval('subthing.method()') == 'method in subclass'
    assert context.eval('Class.prototype.submethod.call(subthing)') == 'submethod'
    assert context.eval('Class.prototype.method.call(subthing)') == 'method in subclass'
    assert context.eval('Superclass.prototype.method.call(subthing)') == 'method in superclass'

def test_super_method_calls(context):
    assert context.eval('superthing.supermethod()') == 'supermethod'
    assert context.eval('superthing.method()') == 'method in superclass'
    assert context.eval('Superclass.prototype.supermethod.call(superthing)') == 'supermethod'
    assert context.eval('Superclass.prototype.method.call(superthing)') == 'method in superclass'
