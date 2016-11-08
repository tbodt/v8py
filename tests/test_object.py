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

def test_converting_objects(Test, context):
    context.glob.test = Test()
    assert context.glob.test.method() == 'thing'
    assert context.eval('test.method()') == 'thing'

