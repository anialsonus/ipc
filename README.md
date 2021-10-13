# IPC

A repository with different IPC implementations to test performance and related problems. At the moment only Unix sockets, UDP and Posix queues are implemented.

For Posix queue we need an additional OS tuning:
```bash
sudo bash -c "echo 65536 > /proc/sys/fs/mqueue/msgsize_max"
```

Then, simply build the server and client pairs:
```bash
mkdir -p build/udp
mkdir -p build/unix
mkdir -p build/queue

gcc src/udp/client.c -o build/udp/client && gcc src/udp/server.c -o build/udp/server
gcc src/unix/client.c -o build/unix/client && gcc src/unix/server.c -o build/unix/server
gcc src/queue/server.c -o build/queue/server -lrt && gcc src/queue/client.c -o build/queue/client -lrt
```

Run in separate shells the server
```bash
./build/unix/server 
```

and the client
```bash
time ./build/unix/client 
```

When client finishes its work, it would output the amount of bytes it has sent (and errors, if any)
```bash
Total bytes: 64512000000

real    1m17.016s
user    0m1.085s
sys     0m33.137s
```

Then send SIGINT (or ^C) to the server and get the amount of consumed bytes
```bash
^CTotal bytes: 64512000000
```
