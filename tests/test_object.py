def test_converting_objects(Test, context):
    context.glob.test = Test()
    assert context.glob.test.method() == 'thing'
    assert context.eval('test.method()') == 'thing'
    print 'test finished'

