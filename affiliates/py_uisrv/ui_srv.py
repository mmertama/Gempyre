import socket
import sys
import subprocess

try:
    import websockets
except ImportError:
    print("Warning: websockets is not installed", file=sys.stderr)

try:
    import webview
except ImportError:
    print("Warning: pywebview is not installed", file=sys.stderr)


def main(port, gui):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((socket.gethostbyname('localhost'), port))
    server_socket.listen(5)
    while True:
        client_socket, address = server_socket.accept()
        data_len = int.from_bytes(client_socket.recv(8), byteorder='big')
        bytes_received = 0
        chunks = []
        while bytes_received < data_len:
            chunk = client_socket.recv(min(data_len - bytes_received, data_len))
            if chunk == b'':
                print("Connection broken", address, data_len, file=sys.stderr)
                break
            chunks.append(str(chunk))
            bytes_received += len(chunk)
        data = ''.join(chunks)
        command = gui + data.split(' ')
        status = subprocess.call(command, stdin=None, stdout=None, stderr=None, shell=False)
        client_socket.send(status.to_bytes(8, byteorder='big', signed=True))
        client_socket.close()


if __name__ == '__main__':
    port = int(sys.argv[1])
    ui = sys.argv[2:]
    main(port, ui)
