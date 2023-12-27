# Utility functions
import os

data_type_dict = {
    '0': 'INT8',
    '1': 'FP16',
    '2': 'FP32',
    '3': 'INT16',
    '4': 'INT32',
    '5': 'BFP16',
    '6': 'INT64',
    '': 'None',
    '-': 'None'
}


data_size_dict = { #prec_map
    '0': 1,
    '1': 2,
    '2': 4,
    '3': 2,
    '4': 4,
    '5': 2,
    '6': 8,
}

def intToHex(dataList):
    newDataList = []
    for data in dataList:
        if not data.isnumeric():
            newDataList.append('')
        else:
            newDataList.append(str(hex(int(data))))
    return newDataList


def get_simulator_total_cycle(simulatorFile):
    simulatorTotalCycle = 0
    if os.path.exists(simulatorFile):
        with open(simulatorFile) as f:
            rows = f.readlines()
            for row in rows:
                if ':' in row:
                    simulatorTotalCycle = int(row.split(': ')[1])
    return simulatorTotalCycle
