def test_convert_to_py(context):
    assert context.eval('"Hello, world!"') == 'Hello, world!'
    assert context.eval('124987234789413') == 124987234789413
    assert context.eval('189329854.18894627') == 189329854.18894627
    assert context.eval('true') == True
    assert context.eval('false') == False
    assert context.eval('null') == None
    assert context.eval('undefined') == None

def test_jsobject(context):
    obj = context.eval('({})')
    obj.foo = 'bar'
    assert obj['foo'] == 'bar'
    obj['foo'] = 'bar'
    assert obj.foo == 'bar'
    assert dir(obj) == ['foo']
    assert str(obj) == '[object Object]'
    # TODO
    # assert vars(obj) == {'foo': 'bar'}

def test_array_getitem(context):
    context.eval('arr = [1, 2, 3]')
    arr = context.glob.arr
    assert arr[0] == 1
    assert arr[1] == 2
    assert arr[2] == 3

def test_glob(context):
    context.eval('foo = "bar"')
    assert context.glob.foo == 'bar'

def test_context_getattr(context):
    context.foo = 'bar'
    assert context.foo == 'bar'
    assert context.glob.foo == 'bar'
    assert context.eval('foo') == 'bar'

def test_convert_to_js(context):
    context.glob.foo = 'Hello, world!'
    assert context.eval('foo == "Hello, world!"')
    context.glob.foo = 13324235
    assert context.eval('foo == 13324235')
    context.glob.foo = 189329854.18894627
    assert context.eval('foo == 189329854.18894627')
    context.glob.foo = True
    assert context.eval('foo == true')
    context.glob.foo = False
    assert context.eval('foo == false')
    context.glob.foo = None
    assert context.eval('foo == null')
    context.glob.foo = None
    assert context.eval('foo == undefined')
