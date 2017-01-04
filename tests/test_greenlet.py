import pytest
import functools
from v8py import Context
try:
    from greenstack import greenlet, GreenletExit
except ImportError:
    pytestmark = pytest.mark.skip(reason="greenlet is not installed")

def test_greenlet():
    def green_func(next_green):
        def swg():
            try: # Kappa
                next_green.switch()
            except GreenletExit: # Kappa
                pass # Kappa
        c = Context()
        c.expose(swg)
        c.eval('swg()')
    g1 = greenlet()
    g2 = greenlet(functools.partial(green_func, greenlet.getcurrent()))
    g1.run = functools.partial(green_func, g2)
    g1.switch()
