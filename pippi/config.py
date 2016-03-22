import os
import shutil

default_path = os.path.dirname(os.path.abspath(__file__)) + '/default.config'

keys = ['device', 'bpm', 'bpmcc', 'divcc', 'divs', 'key', 'ratios', 'a4']

def parse(config_file):
    default_config = {}
    execfile(default_path, default_config)
    if not os.path.isfile(config_file):
        return default_config

    config = {}
    execfile(config_file, config)

    parsed = {}
    for key in keys:
        if key not in config:
            parsed[key] = default_config[key]
        else:
            parsed[key] = config[key]

    return parsed

def init_user_config():
    config_path = os.path.expanduser('~/pippi.config')
    if not os.path.isfile(config_path):
        shutil.copy(default_path, config_path)

    config = {}
    execfile(config_path, config)
    return config

def init():
    """ Look for a config file and load into dict, otherwise load defaults
    """
    config = None
    user_config_path = os.path.expanduser('~/pippi.config')
    local_config_path = os.getcwd() + '/pippi.config'

    # look for a local config file
    if os.path.isfile(user_config_path):
        config = parse(user_config_path)

    if os.path.isfile(local_config_path):
        config = parse(local_config_path)

    if config is None:
        config = init_user_config()

    return config
