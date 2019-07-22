from typing import Set

KEY_CKS_DATA = "cks_data"
KEY_CKS_CONTROL = "cks_control"
KEY_CKR_DATA = "ckr_data"
KEY_CKR_CONTROL = "ckr_control"
KEY_BROADCAST = "broadcast"
KEY_REDUCE_SEND = "reduce_send"
KEY_REDUCE_RECV = "reduce_recv"
KEY_SCATTER = "scatter"
KEY_GATHER = "gather"

DATA_TYPE_SIZE = {
    "char":     1,
    "short":    2,
    "int":      4,
    "float":    4,
    "double":   8
}

PACKET_PAYLOAD_SIZE = 28


class SmiOperation:
    def __init__(self, logical_port: int, data_type: str = "int", buffer_size: int = None):
        self.logical_port = logical_port
        self.data_type = data_type
        self.buffer_size = buffer_size or 16

    def get_channel(self, key: str) -> str:
        inv_map = {v: k for k, v in OP_MAPPING.items()}
        type = inv_map[self.__class__]
        # True to avoid passing program parameters on get_channel, which is used heavily in templates
        # TODO: change
        keys = self.channel_usage(True)
        assert key in keys
        return "{}_{}_{}".format(type, self.logical_port, key)

    def get_channel_defs(self, p2p_rendezvous: bool):
        return [(self.get_channel(ch), self.get_channel_depth(ch)) for ch in sorted(self.channel_usage(p2p_rendezvous))]

    def get_channel_depth(self, channel):
        mapping = {
            "cks_data": 16,
            "cks_control": 16,
            "ckr_data": self.buffer_size,
            "ckr_control": self.buffer_size,
            "broadcast": 1,
            "reduce_send": 1,
            "reduce_recv": 1,
            "scatter": 1,
            "gather": 1
        }
        return mapping[channel]

    def data_size(self):
        return DATA_TYPE_SIZE[self.data_type]

    def data_elements_per_packet(self):
        size = self.data_size()
        return PACKET_PAYLOAD_SIZE // size

    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
        return set()

    def serialize_args(self):
        return {}

    def _signature(self):
        return (self.logical_port, self.data_type, self.buffer_size)

    def __eq__(self, other):
        if other.__class__ != self.__class__:
            return False
        return self._signature() == other._signature()

    def __repr__(self):
        return "{}{}".format(self.__class__.__name__, self._signature())


class Push(SmiOperation):
    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
        if p2p_rendezvous:
            return {KEY_CKS_DATA, KEY_CKR_CONTROL}
        return {KEY_CKS_DATA}


class Pop(SmiOperation):
    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
        if p2p_rendezvous:
            return {KEY_CKR_DATA, KEY_CKS_CONTROL}
        return {KEY_CKR_DATA}


class Broadcast(SmiOperation):
    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_BROADCAST
        }


class Reduce(SmiOperation):
    """
    Maps data type to SHIFT_REG.
    """
    SHIFT_REG = {
        "double": 4,
        "float": 4,
        "int": 1,
        "short": 1,
        "char": 1
    }

    OP_TYPE = {
        "add": "SMI_OP_ADD",
        "max": "SMI_OP_MAX",
        "min": "SMI_OP_MIN"
    }

    SHIFT_REG_INIT = {
        ("char", "add"): "0",
        ("short", "add"): "0",
        ("int", "add"): "0",
        ("float", "add"): "0",
        ("double", "add"): "0",
        ("char", "max"): "CHAR_MIN",
        ("short", "max"): "SHRT_MIN",
        ("int", "max"): "INT_MIN",
        ("float", "max"): "FLT_MIN",
        ("double", "max"): "DBL_MIN",
        ("char", "min"): "CHAR_MAX",
        ("short", "min"): "SHRT_MAX",
        ("int", "min"): "INT_MAX",
        ("float", "min"): "FLT_MAX",
        ("double", "min"): "DBL_MAX"

    }

    def __init__(self, logical_port, data_type="int", buffer_size=None, op_type="add"):
        super().__init__(logical_port, data_type, buffer_size)

        assert data_type in Reduce.SHIFT_REG
        assert op_type in Reduce.OP_TYPE
        self.op_type = op_type

    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
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
        return DATA_TYPE_SIZE[self.data_type]

    def reduce_op(self) -> str:
        return Reduce.OP_TYPE[self.op_type]

    def shift_reg_init(self) -> str:
        return Reduce.SHIFT_REG_INIT[(self.data_type, self.op_type)]

    def serialize_args(self):
        return {
            "op_type": self.op_type
        }

    def _signature(self):
        return (*super()._signature(), self.op_type)


class Scatter(SmiOperation):
    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_SCATTER
        }


class Gather(SmiOperation):
    def channel_usage(self, p2p_rendezvous: bool) -> Set[str]:
        return {
            KEY_CKS_DATA,
            KEY_CKS_CONTROL,
            KEY_CKR_DATA,
            KEY_CKR_CONTROL,
            KEY_GATHER
        }


OP_MAPPING = {
    "push": Push,
    "pop": Pop,
    "broadcast": Broadcast,
    "reduce": Reduce,
    "scatter": Scatter,
    "gather": Gather
}
