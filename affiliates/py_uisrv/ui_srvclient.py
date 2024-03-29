import sys
import socket
import re


def main(params):
    host = re.search(r'host=(\S+)', ' '.join(params))[1]
    port = int(re.search(r'port=(\S+)', ' '.join(params))[1])
    commands = [x for x in params if not re.match(r'(port|host)=', x) and params.index(x) > 0]
    url = commands[0]
    addr = commands[1]
    remote_url = re.sub(r'localhost', addr, url)
    commands = [remote_url] + commands[2:]
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((host, port))
    except ConnectionRefusedError as cre:
        print("Cannot connect to", host, port, cre)
        return -2
    sock.send(len(commands).to_bytes(8, byteorder='big'))
    for command in commands:
        encoded = str.encode(command)
        sock.send(len(encoded).to_bytes(8, byteorder='big'))
        sock.send(encoded)
    return_status = None
    try:
        return_status = sock.recv(8)
    except IOError as e:
        return -1
    sock.close()
    return int.from_bytes(return_status, byteorder='big', signed=True)


if __name__ == "__main__":
    main(sys.argv)


'''

someday do this

parser = argparse.ArgumentParser()
parser.add_argument("--host", type=str)
parser.add_argument("--port", type=int)
parser.add_argument("--address", type=str)
parser.add_argument("--gempyre-url", type=str)
parser.add_argument("--gempyre-width", type=int)
parser.add_argument("--gempyre-height", type=int)
parser.add_argument("--gempyre-title", type=str)
args = parser.parse_args()

host = args.host
port = args.port
commands = [x for x in params if not re.match(r'(port|host)=', x) and params.index(x) > 0]
url = args.gempyre_url
addr = args.address
remote_url = re.sub(r'localhost', addr, url)
commands = [
"--gempyre-url=" + remote_url,
"--gempyre-width=" + args.gempyre_width,
"--gempyre-height=" + args.gempyre_height,
"--gempyre-title=" + args.gempyre_title]


'''
