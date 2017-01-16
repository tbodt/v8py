import pytest
from v8py import Context, Script, JSException

def test_script():
    c1 = Context()
    c1.kappa = 'pride'
    c2 = Context()
    s = Script('kappa')
    assert c1.eval(s) == 'pride'
    with pytest.raises(JSException):
        c2.eval(s)

def test_identical_scripts():
    assert Script('kappa') is Script('kappa')

def test_filename():
    s = Script('kappa', filename='file')
    # why bother testing more...
