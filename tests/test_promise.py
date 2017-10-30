
from v8py import JSPromise

def test_promise(context):
    context.eval('async function p() { return 5; }')
    promise = context.glob.p()
    assert isinstance(promise, JSPromise)