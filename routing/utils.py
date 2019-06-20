from typing import List, TypeVar

T = TypeVar('T')


def round_robin(values: List[T], index: int, size: int) -> List[T]:
    assert size > 0
    assert 0 <= index < size

    return values[index::size]
