import socket
import struct
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

sock.settimeout(2.0) 

server_address = ("127.0.0.1", 7777)

my_number = 42
binary_data = struct.pack('<I', my_number)

while True:
    sock.sendto(binary_data, server_address)
    print(f"Sent: {my_number}")
    
    try:
        response_data, address = sock.recvfrom(1024)
        
        received_number = struct.unpack('<I I', response_data)
                
        print(f"Received back: {received_number} from {address}")
        
    except socket.timeout:
        print("Server didn't respond! (Timeout / Packet Dropped)")
    
    print("-" * 30)
    # time.sleep(1)