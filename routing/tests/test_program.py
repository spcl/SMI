import pytest

from ops import Push, Pop, Broadcast, KEY_CKS_DATA, KEY_CKS_CONTROL, KEY_CKR_DATA, KEY_BROADCAST
from program import Program, FailedAllocation


def test_allocation_fail():
    with pytest.raises(FailedAllocation):
        Program([
            Push(0),
            Broadcast(0)
        ])


def test_allocation_channel_to_ports():
    program = Program([
        Push(0),
        Pop(0),
        Push(1),
        Push(2),
        Pop(2),
        Broadcast(3)
    ])

    ops = program.operations
    assert program.get_channel_allocations(0) == [
        (ops[0], "cks_data"),
        (ops[4], "cks_control"),
        (ops[0], "ckr_control"),
        (ops[4], "ckr_data")
    ]
    assert program.get_channel_allocations(1) == [
        (ops[1], "cks_control"),
        (ops[5], "cks_control"),
        (ops[1], "ckr_data"),
        (ops[5], "ckr_control"),
    ]
    assert program.get_channel_allocations(2) == [
        (ops[2], "cks_data"),
        (ops[5], "cks_data"),
        (ops[2], "ckr_control"),
        (ops[5], "ckr_data"),
    ]
    assert program.get_channel_allocations(3) == [
        (ops[3], "cks_data"),
        (ops[3], "ckr_control"),
    ]


def test_allocation_get_channel():
    program = Program([
        Push(0),
        Pop(0),
        Push(1),
        Push(2),
        Pop(2),
        Broadcast(3)
    ])

    assert program.get_channel_for_port_key(0, KEY_CKS_DATA) == 0
    assert program.get_channel_for_port_key(0, KEY_CKS_CONTROL) == 1
    assert program.get_channel_for_port_key(1, KEY_CKR_DATA) is None
    assert program.get_channel_for_port_key(2, KEY_CKS_DATA) == 3
    assert program.get_channel_for_port_key(3, KEY_CKS_DATA) == 2
    assert program.get_channel_for_port_key(3, KEY_BROADCAST) is None
