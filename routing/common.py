COST_INTER_FPGA = 100
COST_INTRA_FPGA = 1
CHANNELS_PER_FPGA = 4


class Channel:
    def __init__(self, node: str, fpga: str, index: int):
        self.node = node
        self.fpga = fpga
        self.index = index

    def fpga_key(self) -> str:
        return "{}:{}".format(self.node, self.fpga)

    def next_index(self):
        return (self.index + 1) % CHANNELS_PER_FPGA

    def __repr__(self):
        return "{}:{}:{}".format(self.node, self.fpga, self.index)


def write_nodefile(ranks, stream):
    items = sorted(ranks.items(), key=lambda i: i[0])

    for _, node in items:
        stream.write(node)
        stream.write("\n")
