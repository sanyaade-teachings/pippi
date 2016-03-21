import os
import shutil

default_path = os.path.dirname(os.path.abspath(__file__)) + '/default.config'

def init_user_config():
    config_path = os.path.expanduser('~/pippi.config')
    shutil.copy(default_path, config_path)
    return execfile(default_path, {})

def config():
    """ Look for a config file and load into dict, otherwise load defaults
    """
    config = None
    user_config_path = os.path.expanduser('~/pippi.config')
    local_config_path = os.getcwd() + '/pippi.config'

    # look for a local config file
    if os.path.isfile(user_config_path):
        config = execfile(config_file, {})

    if os.path.isfile(local_config_path):
        config = execfile(config_file, {})

    if config is None:
        config = init_user_config()

    return config
