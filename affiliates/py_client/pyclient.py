import os
import asyncio
import json
from urllib.parse import urlparse
import sys
import webview
import websockets

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
    print("on_start", host, port)
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
                    print("do close")
                    await ws.close()
                    return

                doc = receive.result()
                print("web socket received", doc)

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
        
    if len(sys.argv) > 2:
        width = int(sys.argv[2])
        
    if len(sys.argv) > 3:
        height = int(sys.argv[3])
        
    if len(sys.argv) > 4:
        title = sys.argv[4]
    
    uri_string = sys.argv[1]
    
    uri = urlparse(uri_string)

    window = webview.create_window(title, url=uri_string, width=width, height=height)
    window.shown += lambda: on_show(window, uri.hostname, uri.port)
    window.closing += on_close
    webview.start()

    
if __name__ == "__main__":
    main()
