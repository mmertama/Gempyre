import sys
import socket
import re


def main():
    host = re.search(r'host=(\S+)', ' '.join(sys.argv))[1]
    port = int(re.search(r'port=(\S+)', ' '.join(sys.argv))[1])
    command = [x for x in sys.argv if not re.match(r'(port|host)=', x) and sys.argv.index(x) > 0]
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    encoded = str.encode(' '.join(command))
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
    main()

