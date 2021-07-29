import os
import asyncio
import json
from urllib.parse import urlparse
import sys
import webview
import websockets
import re

do_exit = None


# file_types = ('Image Files (*.bmp;*.jpg;*.gif)', 'All files (*.*)')
def make_filters(filters):
    if not filters:
        return tuple()
    filters_list = []
    for k, f in filters.items():
        filter_string = "{} ({})".format(k, ';'.join(f))
        filters_list.append(filter_string)
    return tuple(filters_list)


def on_close():
    if do_exit:
        do_exit()
        os._exit(0)  # socket closing takes 10s! Sorry, Im impatient


def on_show(window, host, port):
    ws_uri = "ws://{}:{}/gempyre".format(host, port)

    async def extender():
        async with websockets.connect(ws_uri, close_timeout=1) as ws:
            loop = asyncio.get_event_loop()
            receive = loop.create_task(ws.recv())

            def f():
                receive.cancel()

            global do_exit
            do_exit = f

            while True:
                try:
                    await receive
                except asyncio.exceptions.CancelledError:
                    await ws.close()
                    return

                doc = receive.result()

                receive = loop.create_task(ws.recv())
                obj = json.loads(doc)
                if not type(obj) is dict:
                    print("Invalid JS object", doc)
                    continue
                if obj['type'] != 'extension':
                    continue

                call_id = obj['extension_call']
                params = json.loads(obj['extension_parameters'])
                ext_id = obj['extension_id']

                if call_id == "openFile":
                    dir_name = params['dir']
                    filters = params["filter"]

                    result = window.create_file_dialog(webview.OPEN_DIALOG,
                                                       directory=dir_name,
                                                       allow_multiple=False,
                                                       file_types=make_filters(filters))
                    response = json.dumps({
                        'type': "extension_response",
                        'extension_call': "openFileResponse",
                        'extension_id': ext_id,
                        'openFileResponse': str(result[0]) if result else ""})
                if call_id == "openFiles":
                    dir_name = params['dir']
                    filters = params["filter"]
                    result = window.create_file_dialog(webview.OPEN_DIALOG,
                                                       directory=dir_name,
                                                       allow_multiple=True,
                                                       file_types=make_filters(filters))
                    response = json.dumps({
                        'type': "extension_response",
                        'extension_call': "openFilesResponse",
                        'extension_id': ext_id,
                        'openFilesResponse': list(result) if result else []})
                if call_id == "openDir":
                    dir_name = params['dir']
                    result = window.create_file_dialog(webview.FOLDER_DIALOG,
                                                       directory=dir_name,
                                                       allow_multiple=False)
                    response = json.dumps({
                        'type': "extension_response",
                        'extension_call': "openDirResponse",
                        'extension_id': ext_id,
                        'openDirResponse': str(result[0]) if result else ""})
                if call_id == "saveFile":
                    dir_name = params['dir']
                    filters = params["filter"]
                    result = window.create_file_dialog(webview.SAVE_DIALOG,
                                                       directory=dir_name,
                                                       allow_multiple=False,
                                                       file_types=make_filters(filters))
                    response = json.dumps({
                        'type': "extension_response",
                        'extension_call': "saveFileResponse",
                        'extension_id': ext_id,
                        'saveFileResponse': str(result) if result else ""})

                await ws.send(response)

    asyncio.run(extender())


def main():
    width = 1024
    height = 768
    title = "Gempyre Application"

    if len(sys.argv) < 2:
        sys.exit("Usage: URL <width> <height> <title>")

    extra = {}
    uri = None

    try:
        if len(sys.argv) > 2:
            width = int(sys.argv[2])

        if len(sys.argv) > 3:
            height = int(sys.argv[3])

        if len(sys.argv) > 4:
            title = sys.argv[4]

        if sys.platform == 'win32':
            extra['gui'] = 'cef'

        if len(sys.argv) > 5:
            print(sys.argv)
            for e in sys.argv[5].split(';'):
                m = re.match(r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*(.*)\s*$', e)
                extra[m[1]] = m[2]

        uri_string = sys.argv[1]
        uri = urlparse(uri_string)

    except ValueError as err:
        print("Invalid parameters", err, sys.argv, file=sys.stderr)
        sys.exit(-1)

    window = webview.create_window(title, url=uri_string, width=width, height=height)
    window.shown += lambda: on_show(window, uri.hostname, uri.port)
    window.closing += on_close
    webview.start(**extra)

    
if __name__ == "__main__":
    main()

'''
someday to this:
parser = argparse.ArgumentParser()
parser.add_argument("--gempyre-url", type=str)
parser.add_argument("--gempyre-width", type=int)
parser.add_argument("--gempyre-height", type=int)
parser.add_argument("--gempyre-title", type=str)
parser.add_argument("--gempyre-extra", type=str)
args = parser.parse_args()

if not args.gempyre_url:
    parser.error(-1)

extra = {}
uri = None

try:
    if args.gempyre_width:
        width = int(args.gempyre_width)

    if args.gempyre_height:
        width = int(args.gempyre_height)

    if args.gempyre_title:
       title = args.gempyre_title

    if sys.platform == 'win32':
        extra['gui'] = 'cef'

    if args.gempyre_extra:
        print(args.gempyre_extra)
        for e in args.gempyre_extra.split(';'):
            m = re.match(r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*(.*)\s*$', e)
            extra[m[1]] = m[2]

'''
