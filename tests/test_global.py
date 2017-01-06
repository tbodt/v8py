import sys
import pytest
from v8py import Context

if sys.version_info.major < 3:
    params = ['new', 'old']
else:
    params = ['new']
@pytest.fixture(params=['new', 'old'])
def Global(request):
    if request.param == 'new':
        class Global(object):
            def method(self):
                return 'method'
            @property
            def property(self):
                return 'property'
        return Global
    else:
        class Global(object):
            def method(self):
                return 'method'
            @property
            def property(self):
                return 'property'
        return Global

@pytest.fixture
def context(Global):
    return Context(Global)

def test_global(context):
    assert context.eval('method()') == 'method'
    assert context.eval('this.method()') == 'method'

    assert context.eval('property') == 'property'
    assert context.eval('this.property') == 'property'
