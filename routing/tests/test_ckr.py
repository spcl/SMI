from common import FPGA
from table import ckr_routing_table

CHANNELS_PER_FPGA = 4


def test_cks_table():
    tag_count = 9
    fpga = FPGA("n", "f")
    for i in range(CHANNELS_PER_FPGA):
        fpga.add_channel(i)

    assert ckr_routing_table(fpga.channels[0], CHANNELS_PER_FPGA, tag_count) == [4, 1, 2, 3, 5, 1, 2, 3, 6]
