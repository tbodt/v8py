from v8py import JSObject

def test_jsfunction(context):
    f = context.eval('Math.sqrt')
    assert isinstance(f, JSObject)
