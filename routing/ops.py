from typing import Tuple, Set

KEY_CKS_DATA = ("cks", "data")
KEY_CKS_CONTROL = ("cks", "control")
KEY_CKR_DATA = ("ckr", "data")
KEY_CKR_CONTROL = ("ckr", "control")
KEY_BROADCAST = "broadcast"
KEY_REDUCE = "reduce"


class SmiOperation:
    def __init__(self, logical_port):
        self.logical_port = logical_port

    def hw_port_usage(self) -> Set[Tuple[str, str]]:
        return set()


class Push(SmiOperation):
    def hw_port_usage(self) -> Set[Tuple[str, str]]:
        return {KEY_CKS_DATA, KEY_CKR_CONTROL}

    def __repr__(self):
        return "Push({})".format(self.logical_port)


class Pop(SmiOperation):
    def hw_port_usage(self) -> Set[Tuple[str, str]]:
        return {KEY_CKR_DATA, KEY_CKS_CONTROL}

    def __repr__(self):
        return "Pop({})".format(self.logical_port)


class Broadcast(SmiOperation):
    def hw_port_usage(self) -> Set[Tuple[str, str]]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_BROADCAST
        }

    def __repr__(self):
        return "Broadcast({})".format(self.logical_port)


class Reduce(SmiOperation):
    def hw_port_usage(self) -> Set[Tuple[str, str]]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_REDUCE
        }

    def __repr__(self):
        return "Broadcast({})".format(self.logical_port)
