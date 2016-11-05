import pytest
from v8py import Context

@pytest.fixture
def context():
    return Context()

