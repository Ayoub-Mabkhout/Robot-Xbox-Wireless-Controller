import time
import socket
from controller import XboxController

# IP address and port of the server
SERVER_IP = "10.121.44.218"
SERVER_PORT = 8888

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Set the timeout for the socket to 1 second
sock.settimeout(5)

# Set the remote device IP and port
server_address = (SERVER_IP, SERVER_PORT)

# Send the message "INIT" to the remote device
sock.sendto(b'INIT', server_address)

# Wait for the response "ACK" from the remote device
try:
    data, server = sock.recvfrom(1024)
    if data.decode() == "ACK":
        print("Connection established")
        joy = XboxController()

    # Send "Hello World" every 1/30th of a seconda without expecting any response
    while True:
        inp = joy.read()
        data = ""
        for d in inp:
            data += str(d) + " "
        data = data.rstrip()
        sock.sendto(data.encode(), server_address)
        time.sleep(1/30)

except socket.timeout:
    print("Connection timeout")



# Close the socket
sock.close()