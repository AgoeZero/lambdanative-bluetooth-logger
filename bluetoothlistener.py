import socket, traceback

print("Creating Socket")
s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
print("Binding")
s.bind((socket.BDADDR_ANY, 1))
print("Listening")
s.listen(1)

try:
	client, addr = s.accept()
	print(client, addr)
	print("1 Connection, sending bytes")
	#s.send(b'Yes, the app is working!')
	print("Receiving from client")
	#data = client.recv(1024)
	#print(data)
	client.send(input("DATA: ").encode("utf-8"))
except OSError:
	traceback.print_exc()
finally:
	s.close()
