#!/bin/env python
import socket, ssl, difflib

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ssl_context = ssl.create_default_context()
ssl_sock = ssl_context.wrap_socket(sock, server_hostname="api.telegram.org")
ssl_sock.connect(("api.telegram.org", 443))

req = b"POST /bot5267382972:AAFYur5ovlMcLm1ClW2PEnD-oW947NDzWKw/sendDocument HTTP/1.1\r\nHost: api.telegram.org\r\nContent-Length: 298\r\nContent-Type: multipart/form-data; boundary=------------------------b73b76428e9a90f6\r\n\r\n--------------------------b73b76428e9a90f6\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n84961485\r\n--------------------------b73b76428e9a90f6\r\nContent-Disposition: form-data; name=\"document\"; filename=\"test.txt\"\r\nContent-Type: text/plain\r\n\r\nciao\r\n\r\n--------------------------b73b76428e9a90f6--\r\n"
with open("reqw", "wb") as f:
    f.write(req)
print(req)
ssl_sock.send(req)
ssl_sock.recv(4096)

ssl_sock.close()

