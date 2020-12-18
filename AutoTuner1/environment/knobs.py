# -*- coding: utf-8 -*-
"""
desciption: Knob information

"""

import utils
import configs
import collections

# 8GB
memory_size = 1024*1024*1024*1024
# 100GB
disk_size = 100*1024*1024*1024
instance_name = ''


KNOBS = [
    #Knobs of leveldb
        'create_if_missing',
        'write_buffer_size',
        'block_size',
        'max_open_files',
        'max_file_size',
        # 'block_cache',
        # 'compression',
         ]

KNOB_DETAILS = None
EXTENDED_KNOBS = None
num_knobs = len(KNOBS)


def init_knobs(instance, num_more_knobs):
    global instance_name
    global memory_size
    global disk_size
    global KNOB_DETAILS
    global EXTENDED_KNOBS
    instance_name = instance
    # TODO: Test the request
    use_request = False
    if use_request:
        if instance_name.find('tencent') != -1:
            memory_size, disk_size = utils.get_tencent_instance_info(instance_name)
        else:
            memory_size = configs.instance_config[instance_name]['memory']
            #disk_size = configs.instance_config[instance_name]['disk']
    else:
        memory_size = configs.instance_config[instance_name]['memory']
        #disk_size = configs.instance_config[instance_name]['disk']

    KNOB_DETAILS = {
    #Knobs of leveldb
        'create_if_missing': ['boolen', ['FALSE', 'TRUE']],
        'write_buffer_size': ['integer', [0, 1000000000, 4194304]],
        'block_size': ['integer', [0, 4096, 4096]],
        'max_open_files': ['integer', [0, 1000, 1000]],
        'max_file_size': ['integer', [0, 3000000, 2097152]],
        #'block_cache': [],
        #'compression': [],
    }

    # TODO: ADD Knobs HERE! Format is the same as the KNOB_DETAILS
    UNKNOWN = 0
    EXTENDED_KNOBS = {
    }
    # ADD Other Knobs, NOT Random Selected
    i = 0
    EXTENDED_KNOBS = dict(sorted(EXTENDED_KNOBS.items(), key=lambda d: d[0]))
    for k, v in EXTENDED_KNOBS.items():
        if i < num_more_knobs:
            KNOB_DETAILS[k] = v
            KNOBS.append(k)
            i += 1
        else:
            break
    print("Instance: %s Memory: %s" % (instance_name, memory_size))


def get_init_knobs():

    knobs = {}

    for name, value in KNOB_DETAILS.items():
        # 0是类型,1是值
        knob_value = value[1]
        # 最后一位是默认值
        knobs[name] = knob_value[-1]

    return knobs


def gen_continuous(action):
    knobs = {}

    for idx in xrange(len(KNOBS)):
        name = KNOBS[idx]
        value = KNOB_DETAILS[name]
        knob_type = value[0]
        knob_value = value[1]
        min_value = knob_value[0]

        if knob_type == 'integer':
            max_val = knob_value[1]
            eval_value = int(max_val * action[idx])
            eval_value = max(eval_value, min_value)

        else:
            enum_size = len(knob_value)
            enum_index = int(enum_size * action[idx])
            enum_index = min(enum_size - 1, enum_index)
            eval_value = knob_value[enum_index]

        knobs[name] = eval_value
    return knobs


def save_knobs(knob, metrics, knob_file):
    """ Save Knobs and their metrics to files
    Args:
        knob: dict, knob content
        metrics: list, tps and latency
        knob_file: str, file path
    """
    # format: tps, latency, knobstr: [#knobname=value#]
    knob_strs = []
    for kv in knob.items():
        knob_strs.append('{}:{}'.format(kv[0], kv[1]))
    result_str = '{},{},{},'.format(metrics[0], metrics[1], metrics[2])
    knob_str = "#".join(knob_strs)
    result_str += knob_str

    with open(knob_file, 'a+') as f:
        f.write(result_str+'\n')

