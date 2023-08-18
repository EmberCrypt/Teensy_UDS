from ICS_VSBIO import VSBReader as reader

class Can_Msg:

    TYPE_SM = 0
    TYPE_FM = 1
    TYPE_CS = 2
    TYPE_FC = 3

    DIAG_ID_LIM = 0x4c0

    def __init__(self, arb_id, data, msg_type, n_bytes = 0):
        self.arb_id = arb_id
        self.data = data
        self.type = msg_type

        if n_bytes ==0:
            self.n_bytes = len(self.data)
        else:
            self.n_bytes = n_bytes

    def from_line(line):
        '''
            Line of the following format:
            can_id,d0 d1 d2 d3 d4 d5 ... dn
        '''
        arb_id, data = line.split(",")
        arb_id = int(arb_id, 16)
        data = [int(x, 16) for x in data.split(" ")]

        return Can_Msg(arb_id, data, 0)

    def from_vsb(vsb_msg):
        '''
            Initialise from vsb message
            TODO also from_vsb
        '''

        arb_id = vsb_msg.info.ArbIDOrHeader

        if arb_id < Can_Msg.DIAG_ID_LIM:
            return None

        n_bytes = 0x0
        data = []

        # tmp
        data = []
        for i in range(8):
            data.append(vsb_msg.get_byte_from_data(i))

        # easy
        if data[0] & 0xf0 == 0x0:
            n_bytes = data[0]
            data = data[1:n_bytes + 1]
            t = Can_Msg.TYPE_SM
        elif data[0] & 0xf0 == 0x10:
            n_bytes = ((data[0] & 0x0f) << 8) | data[1]
            data = data[2:]
            t = Can_Msg.TYPE_FM
        elif data[0] & 0xf0 == 0x20:
            data = data[1:]
            t = Can_Msg.TYPE_CS
        elif data[0] & 0xf0 == 0x30:
            data = data
            t = Can_Msg.TYPE_FC
        return Can_Msg(arb_id, data, t, n_bytes)

    def __str__(self):
        res = f"{self.arb_id:04x}," + " ".join([f"{b:02x}" for b in self.data])
        return res

    def __hash__(self):
        return hash(str(self))

    def __eq__(self, other):
        # TODO weird bug
        return True

    def from_list(file):
        can_msgs = []
        vsbRead = reader.VSBReader(file)
        cs = None
        try:
            for msg in vsbRead:
                m = Can_Msg.from_vsb(msg)
                if m and m.arb_id >= Can_Msg.DIAG_ID_LIM:
                    # multiple frames
                    if m.type == Can_Msg.TYPE_SM:
                        can_msgs.append(m)
                    elif m.type == Can_Msg.TYPE_FM:
                        cs = m
                    if cs and cs.arb_id == m.arb_id:
                        cs.data += m.data
                        if len(cs.data) >= cs.n_bytes:
                            can_msgs.append(cs)
                            cs = None
                        

        except ValueError as e:
            print(str(e))
        except Exception as ex:
            print(ex)
        return can_msgs

