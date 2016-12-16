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
