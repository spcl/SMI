import pytest

from ops import Push, Pop, Broadcast
from program import Program, FailedAllocation


def test_allocation_fail():
    with pytest.raises(FailedAllocation):
        Program(4096, [
            Push(0),
            Push(0)
        ])


def test_allocation_overlap():
    program = Program(4096, [
        Push(0),
        Broadcast(1)
    ])

    group = program.create_group(("cks", "data"))
    assert group.hw_port_count() == 2
    assert group.hw_mapping() == [0, 1]

    group = program.create_group(("cks", "control"))
    assert group.hw_port_count() == 1
    assert group.hw_mapping() == [-1, 0]


def test_allocation_channel_to_ports():
    program = Program(4096, [
        Push(0),
        Pop(0),
        Push(1),
        Push(2),
        Pop(2)
    ])

    assert program.get_channel_allocations(0) == {
        "cks": [("data", 0), ("control", 1)],
        "ckr": [("data", 0), ("control", 2)]
    }
    assert program.get_channel_allocations(1) == {
        "cks": [("data", 1)],
        "ckr": [("data", 1)]
    }
    assert program.get_channel_allocations(2) == {
        "cks": [("data", 2)],
        "ckr": [("control", 0)]
    }
    assert program.get_channel_allocations(3) == {
        "cks": [("control", 0)],
        "ckr": [("control", 1)]
    }


def test_allocation_get_channel():
    program = Program(4096, [
        Push(0),
        Pop(0),
        Push(1),
        Push(2),
        Pop(2)
    ])

    assert program.get_channel_for_logical_port(0, ("cks", "data")) == 0
    assert program.get_channel_for_logical_port(0, ("cks", "control")) == 3
    assert program.get_channel_for_logical_port(1, ("ckr", "data")) is None
    assert program.get_channel_for_logical_port(2, ("cks", "data")) == 2
