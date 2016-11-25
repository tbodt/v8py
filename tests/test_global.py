import pytest
from v8py import Context

@pytest.fixture(params=['old', 'new'])
def Global(request):
    if request.param == 'new':
        class Global(object):
            def method(self):
                return 'method'
        return Global
    else:
        class Global(object):
            def method(self):
                return 'method'
        return Global

@pytest.fixture
def context(Global):
    return Context(Global)

def test_global(context):
    assert context.eval('method()') == 'method'
