def test_convert_to_py(context):
    assert context.eval('"Hello, world!"') == 'Hello, world!'
    assert context.eval('124987234789413') == 124987234789413
    assert context.eval('189329854.18894627') == 189329854.18894627
    assert context.eval('true') == True
    assert context.eval('false') == False
    assert context.eval('null') == None
    assert context.eval('undefined') == None
    assert context.eval('[1, 2, 3]') == [1, 2, 3]

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
    context.glob.foo = [1, 2, 3]
    # In JavaScript, [1,2,3] == [1,2,3] is false. Don't ask.
    assert context.eval('foo.length == 3')
    assert context.eval('foo[0] == 1')
    assert context.eval('foo[1] == 2')
    assert context.eval('foo[2] == 3')
    # assert context.eval('foo == [1, 2, 3]')
