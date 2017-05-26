import importlib.util
from importlib import reload
import os
import sys
import re
import glob
import logging

import urwid
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

logging.basicConfig(filename='pippi.log', level=logging.INFO)

class InstrumentNotFoundError(Exception):
    def __init__(self, instrument_name, *args, **kwargs):
        self.message = 'No instrument named %s found' % instrument_name

class InstrumentHandler(FileSystemEventHandler):
    def __init__(self, *args, **kwargs):
        basepath = os.path.realpath(os.path.curdir)
        self.orcdir = os.path.join(basepath, 'orc')
        logging.info('orcdir %s' % self.orcdir)
        #if os.path.exists(self.orcdir):
        #    sys.path.append(self.orcdir)

        #logging.info('sys.path %s' % sys.path)

        self.instruments = {}
        self.load_instruments()
        super(InstrumentHandler, self).__init__(*args, **kwargs)

    def add_instrument(self, name, path=None):
        if path is None:
            path = self.orcdir
        logging.info('add_instrument %s, %s' % (name, path))
        try:
            instrument_spec = importlib.util.spec_from_file_location(name, path)
            if instrument_spec is not None:
                instrument = importlib.util.module_from_spec(instrument_spec)
                instrument_spec.loader.exec_module(instrument)
                self.instruments[instrument_name] = instrument
        except ModuleNotFoundError as e:
            raise InstrumentNotFoundError(name) from e

    def load_instruments(self):
        for instrument_path in glob.glob(os.path.join(self.orcdir, '*.py')):
            instrument_name = os.path.basename(instrument_path).replace('.py', '')
            logging.info('instrument path %s' % instrument_path)
            try:
                self.add_instrument(instrument_name, instrument_path)
            except InstrumentNotFoundError as e:
                logging.info(e.message)

    def on_modified(self, event):
        path = event.src_path
        if path in self.instruments:
            reload(self.instruments[path])

RE_ADD_VOICE = r'^p\s+'

class PippiConsole(urwid.Pile):
    def __init__(self, *args, **kwargs):
        self.palette = [
            ('voice', 'white', 'dark green'), 
            ('instrument', 'white', 'dark blue'), 
            ('prompt', 'light blue', 'black'), 
            ('message', 'black', 'light green'), 
            ('param', 'white', 'dark magenta'), 
            ('black', 'black', 'black'), 
        ]

        header_rows = 30

        self.commands = []
        self.history_pos = -1
        self.prompt = urwid.Edit([('prompt', '^_-'), ('black', ' ')])

        self.voices = urwid.Pile([urwid.Text(('voice', 'Voices'))])
        self.instruments = urwid.Pile([urwid.Text(('instrument', 'Instruments'))])

        self.params = urwid.AttrMap(urwid.Text(('param', '')), 'param')
        self.message = urwid.AttrMap(urwid.Text(('message', '')), 'message')

        self.instrument_handler = InstrumentHandler()
        self.instrument_observer = Observer()
        self.instrument_observer.schedule(self.instrument_handler, path='.', recursive=True)

        self.cmd_patterns = {
            RE_ADD_VOICE: re.compile(RE_ADD_VOICE), 
        }

        self.columns = urwid.Columns((
            self.voices,
            #(1, urwid.SolidFill('\u2502')), 
            self.instruments,
        )) 

        widgets = (self.columns, self.params, self.message, self.prompt)
        self.instrument_observer.start()

        super(PippiConsole, self).__init__(widgets)

    def handle_cmd(self, cmd):
        if cmd in ('q', 'quit', 'exit'):
            self.quit()
        else:
            # parse commands
            if self.cmd_patterns[RE_ADD_VOICE].match(cmd):
                cmd = self.cmd_patterns[RE_ADD_VOICE].sub('', cmd)
                voice_name = self.add_voice(cmd)

                voice_name = 'fixed'
                if voice_name is not None:
                    self.voices.contents += [ (urwid.Text(('voice', 'Voice %s' % voice_name)), self.voices.options()) ]

    def add_voice(self, cmd):
        # TODO potentially parse extra params here
        cmds = cmd.split(' ')
        instrument_name = cmds.pop(0)
        logging.info('Adding %s voice' % instrument_name)

        if instrument_name not in self.instrument_handler.instruments:
            logging.info('Loading %s instrument' % instrument_name)
            try:
                self.instrument_handler.add_instrument(instrument_name)
                logging.info('%s instrument loaded' % instrument_name)
                self.instruments.contents += [ (urwid.Text(('instrument', 'Inst %s' % instrument_name)), self.instruments.options()) ]
                self.message.original_widget.set_text(('message', '%s loaded' % instrument_name))

            except InstrumentNotFoundError as e:
                logging.info(e.message)
                self.message.original_widget.set_text(('message', e.message))
                return None

        # now start the playback process

        return instrument_name

    def keypress(self, size, key):
        super(PippiConsole, self).keypress(size, key)
        if key == 'enter':
            cmd = self.prompt.get_edit_text()
            self.handle_cmd(cmd)
            self.prompt.set_edit_text('')
            self.commands += [ cmd ]

        elif key == 'up':
            self.history_pos += 1
            try:
                cmd = self.commands[self.history_pos]
                self.prompt.set_edit_text(cmd)
            except IndexError:
                pass
        elif key == 'down':
            self.history_pos -= 1
            try:
                cmd = self.commands[self.history_pos]
                self.prompt.set_edit_text(cmd)
            except IndexError:
                pass

        else:
            self.history_pos = 0

    def quit(self):
        self.instrument_observer.stop()
        raise urwid.ExitMainLoop()


console = PippiConsole()
loop = urwid.MainLoop(urwid.Filler(console, 'bottom'), console.palette)

try:
    loop.run()
except KeyboardInterrupt as e:
    loop.screen.stop()
    console.quit()


