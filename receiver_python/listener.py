import socket as s

sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))

while True:
    data, addr = sock.recvfrom(2048)
    print(addr, data)
