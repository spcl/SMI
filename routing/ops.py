from typing import Tuple, Set


class SmiOperation:
    def __init__(self, logical_port):
        self.logical_port = logical_port

    def uses_cks_data(self) -> bool:
        return False

    def uses_cks_control(self) -> bool:
        return False

    def uses_ckr_data(self) -> bool:
        return False

    def uses_ckr_control(self) -> bool:
        return False

    def uses_bcast_channel(self) -> bool:
        return False

    def hw_port_usage(self) -> Set[Tuple[str, str]]:
        usage = set()
        if self.uses_cks_data():
            usage.add(("cks", "data"))
        if self.uses_cks_control():
            usage.add(("cks", "control"))
        if self.uses_ckr_data():
            usage.add(("ckr", "data"))
        if self.uses_ckr_control():
            usage.add(("ckr", "control"))
        if self.uses_bcast_channel():
            usage.add("broadcast")
        return usage


class Push(SmiOperation):
    def uses_cks_data(self) -> bool:
        return True

    def uses_ckr_control(self) -> bool:
        return True

    def __repr__(self):
        return "Push({})".format(self.logical_port)


class Pop(SmiOperation):
    def uses_ckr_data(self) -> bool:
        return True

    def uses_cks_control(self) -> bool:
        return True

    def __repr__(self):
        return "Pop({})".format(self.logical_port)


class Broadcast(SmiOperation):
    def uses_cks_data(self) -> bool:
        return True

    def uses_cks_control(self) -> bool:
        return True

    def uses_ckr_data(self) -> bool:
        return True

    def uses_ckr_control(self) -> bool:
        return True

    def uses_bcast_channel(self) -> bool:
        return True

    def __repr__(self):
        return "Broadcast({})".format(self.logical_port)
