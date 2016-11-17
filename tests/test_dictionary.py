import pytest

@pytest.fixture
def o(context):
    return context.eval('o = {foo: "bar"}')

def test_keys(o):
    assert o.keys() == ['foo']

def test_length(o):
    assert len(o) == 1

def test_getitem(o):
    assert o['foo'] == 'bar'

def test_setitem(o):
    o['foo'] = 'baz'
    assert o['foo'] == 'baz'

def test_delitem(o):
    del o['foo']
    assert len(o) == 0
    assert dict(o) == {}

def test_dictification(o):
    assert dict(o) == {'foo': 'bar'}
