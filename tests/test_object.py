import pytest

@pytest.fixture
def obj(context):
    return context.eval('({})')

def test_attrs(obj):
    obj.foo = 'bar'
    assert obj.foo == 'bar'
def test_items(obj):
    obj['foo'] = 'bar'
    assert obj['foo'] == 'bar'

def test_dir(context, obj):
    obj.foo = 'bar'
    assert dir(obj) == ['foo']

def test_str(obj):
    assert str(obj) == '[object Object]'
    assert repr(obj) == '[object Object]'

