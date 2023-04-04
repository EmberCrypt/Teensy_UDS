from teensy_cs import Teensy_CS


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

    def raw_uds(self, data, n_bytes = 0):
        ''' TODO maybe put this in a separate class? '''
        if n_bytes == 0:
            n_bytes = len(data)
        self.teensy.write([self.CMD_RAW_UDS])
        self.teensy.write(n_bytes.to_bytes(2, 'little'))
        self.teensy.write(data[:n_bytes])
        _r = self.read(self.CMD_RAW_UDS)
        return _r

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
        self.teensy.write(n_bytes.to_bytes(2, 'little'))
        self.teensy.write(data[:n_bytes])

        _r = self.read(self.CMD_UDS_SEND)
        return _r

    def uds_rx(self):
        return self.run_cmd(self.CMD_UDS_RECV)



if __name__ == "__main__":
    pass    




