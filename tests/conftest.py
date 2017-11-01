import pytest
from v8py import Context

@pytest.fixture
def context():
    return Context()

@pytest.fixture
def context_with_timeout():
    return Context(timeout=0.1)

