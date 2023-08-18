import can_msg
from teensy_cs import Teensy_CS


TEENSY_MODE_UDS = 0
TEENSY_MODE_CAN = 1


class Teensy_UDS(Teensy_CS):

    '''
        These require uds_target.cpp to be flashed
    '''
    CMD_RAW_UDS	    =   0x21
    CMD_TEST        =   0x22
    CMD_SEC_ACCESS  =   0x23
    CMD_CAN_SNIFF   =   0x24
    CMD_UDS_SEND    =   0x25
    CMD_UDS_RECV    =   0x26
    CMD_SET_IDS     =   0x27
    CMD_RAW_CAN     =   0x28
    CMD_SET_MODE    =   0x29

    def _write_additional_bytes(self, data):
        if len(data) > 0xffff:
            raise ValueError

        self.teensy.write(len(data).to_bytes(2, 'little'))
        self.teensy.write(data)

    def set_mode(self, mode):
        self.teensy.write([self.CMD_SET_MODE])
        self.teensy.write(mode.to_bytes(1, 'little'))
        return self.read(self.CMD_SET_MODE)

    def raw_uds(self, data, n_bytes = 0):
        ''' TODO maybe put this in a separate class? '''
        if n_bytes == 0:
            n_bytes = len(data)
        self.teensy.write([self.CMD_RAW_UDS])
        self._write_additional_bytes(data[:n_bytes])
        _r = self.read(self.CMD_RAW_UDS)
        return _r

    def raw_can(self, can_id, data):
        self.teensy.write([self.CMD_RAW_CAN])
        d = [(can_id & 0xff), (can_id >> 0x8) & 0xff] + data
        self._write_additional_bytes(d)
        return self.read(self.CMD_RAW_CAN)

    def sec_access(self, level):
        ''' TODO separate class & incorporate level '''
        self.teensy.write([self.CMD_SEC_ACCESS])
        _r = self.read(self.CMD_SEC_ACCESS)
        return _r

    def set_ids(self, r_id, w_id):
        size = 4
        self.teensy.write([self.CMD_SET_IDS])
        self.teensy.write(size.to_bytes(2, 'little'))
        self.teensy.write(r_id.to_bytes(2, 'little'))
        self.teensy.write(w_id.to_bytes(2, 'little'))

        _r = self.read(self.CMD_SET_IDS)
        return _r

    def uds_tx(self, data, n_bytes = 0):
        if n_bytes == 0:
            n_bytes = len(data)
        self.teensy.write([self.CMD_UDS_SEND])
        self._write_additional_bytes(data[:n_bytes])
        return self.read(self.CMD_UDS_SEND)

    def uds_rx(self):
        ''' Returns Can_Msg object '''
        _r = self.run_cmd(self.CMD_UDS_RECV)
        if not _r[0]:
            return None

        can_id = int.from_bytes(_r[1:3], 'little')
        data = _r[3:]

        return can_msg.Can_Msg(can_id, data, 0)
        



if __name__ == "__main__":
    pass    




