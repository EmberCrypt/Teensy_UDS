import can_msg

class Ecu_DB:
    ''' 
        Based on an ECU which has one UDS rx id and one UDS tx id.
    '''

    def __init__(self, db_file):
        self.db = {}
        self.from_file(db_file)


    def from_file(self, db_file):
        '''
            File format:
            rx_id
            tx_id
            req_0
            resp_0
            req_1
            resp_1
            ...
        '''
        with open(db_file, "r") as f:
            lines = f.read().splitlines()
        self.rx_id = int(lines[0], 16)
        self.tx_id = int(lines[1], 16)

        for i in range(2, len(lines) - 2, 2):
            self.db[can_msg.Can_Msg.from_line(lines[i])] = can_msg.Can_Msg.from_line(lines[i + 1])

        for msg in self.db:
            print(f"{msg} -> {self.db[msg]}")

    def process(self, can_msg):

        if can_msg in self.db:
            return self.db[can_msg]

        return None

