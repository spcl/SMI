from typing import Set

KEY_CKS_DATA = "cks_data"
KEY_CKS_CONTROL = "cks_control"
KEY_CKR_DATA = "ckr_data"
KEY_CKR_CONTROL = "ckr_control"
KEY_BROADCAST = "broadcast"
KEY_REDUCE_SEND = "reduce_send"
KEY_REDUCE_RECV = "reduce_recv"
KEY_SCATTER = "scatter"


class SmiOperation:
    def __init__(self, logical_port):
        self.logical_port = logical_port

    def hw_port_usage(self) -> Set[str]:
        return set()


class Push(SmiOperation):
    def hw_port_usage(self) -> Set[str]:
        return {KEY_CKS_DATA, KEY_CKR_CONTROL}

    def __repr__(self):
        return "Push({})".format(self.logical_port)


class Pop(SmiOperation):
    def hw_port_usage(self) -> Set[str]:
        return {KEY_CKR_DATA, KEY_CKS_CONTROL}

    def __repr__(self):
        return "Pop({})".format(self.logical_port)


class Broadcast(SmiOperation):
    def hw_port_usage(self) -> Set[str]:
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
    """
    Maps data type to SHIFT_REG.
    """
    SHIFT_REG = {
        "float": 4,
        "int": 1
    }
    DATA_SIZE = {
        "float": 4,
        "int": 4
    }

    def __init__(self, logical_port, data_type):
        super().__init__(logical_port)

        assert data_type in Reduce.SHIFT_REG
        self.data_type = data_type

    def hw_port_usage(self) -> Set[str]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_REDUCE_SEND,
            KEY_REDUCE_RECV
        }

    def shift_reg(self) -> int:
        return Reduce.SHIFT_REG[self.data_type]

    def data_size(self) -> int:
        return Reduce.DATA_SIZE[self.data_type]

    def __repr__(self):
        return "Broadcast({})".format(self.logical_port)


class Scatter(SmiOperation):
    def hw_port_usage(self) -> Set[str]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_SCATTER
        }

    def __repr__(self):
        return "Scatter({})".format(self.logical_port)

