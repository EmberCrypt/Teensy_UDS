import teensy_uds
import logging
import time
import argparse

import ecu_db


class Ecu_Simulator:

    def __init__(self, db_file):
        # TODO can also implement this for multiple DB files
        self.db = ecu_db.Ecu_DB(db_file)

        self.ecu = teensy_uds.Teensy_UDS(log = 0)
        # UDS mode
        self.ecu.set_mode(teensy_uds.TEENSY_MODE_UDS)
        self.ecu.set_ids(self.db.rx_id, self.db.tx_id)

        # Initialise target
        self.ecu.setup_target()

    def run(self):
        while 1:
            _r = self.ecu.uds_rx()
            if _r:
                print(_r)
                _t = self.db.process(_r)
                if _t:
                    print(_t)
                    self.ecu.uds_tx(_t.data, len(_t.data))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('file', type=str)
    args = parser.parse_args()

    e = Ecu_Simulator(args.file)
    e.run()
    logging.basicConfig(level=logging.DEBUG, format="%(filename)s:%(funcName)s: %(message)s")

