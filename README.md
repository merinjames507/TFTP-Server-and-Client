# TFTP(Trivial File Transfer Protocol)
A C-based TFTP client and server implemented using Linux UDP socket programming. Features concurrent client handling via process forking, dynamic ports, 512-byte data block transfers, error reporting, and lock-step timeout retransmissions.

---

## Features

- UDP Socket Communication
- File Upload (PUT)
- File Download (GET)
- Supports **Octet** and **NetASCII** transfer modes
- Interactive command-line client
- Error handling & Timeout Recovery
- Configurable transfer mode

---

## Technologies Used

- C Programming
- Linux
- UDP Socket Programming
- GCC
- POSIX System Calls

---

## Project Structure

```
TFTP/
├── Client/
   │── tftp_client.c            # Client implementation
   │── tftp_client.h             # Client header
   |── tftp_client_file.c        # Client receive and send files

|── Common/ 
   │── tftp.c               # Common functions
   │── tftp.h               # Common definitions

|── Server/
   │── tftp_server.c             # Server implementation
   │── tftp_server_file.c        # Server receive and send files
   │── receive_folder/           # Downloaded files 

│── client.out           # Client executable
│── server.out           # Server executable

```

---

## Supported Operations

### Connect

Connects the client to the TFTP server.

### PUT

Uploads a file from the client to the server.

### GET

Downloads a file from the server.

### Mode Selection

Supports:

- Default
- Octet (Binary Mode)
- NetASCII (Text Mode)

---

## Compilation

Compile the server:

```bash
gcc tftp.c tftp_server.c -o server.out
```

Compile the client:

```bash
gcc tftp.c tftp_client.c -o client.out
```

---

## Running the Project

### Start the Server

```bash
./server.out
```

### Start the Client

```bash
./client.out
```

---

## Client Menu

```
1. Connect
2. PUT
3. GET
4. Mode
5. Exit/ Bye
```

---

## Working

1. Start the server.
2. Run the client.
3. Connect to the server by providing the server IP and port.
4. Select the desired operation:
   - Upload a file (PUT)
   - Download a file (GET)
5. Choose the transfer mode if required.
6. Files received from the server are stored in the `receive_folder` directory.

---

## Concepts Demonstrated

- UDP Socket Programming
- Client-Server Architecture
- File Handling
- Packet-Based Communication
- Network Programming
- TFTP Protocol Basics
- Error Handling
- Linux System Calls

---

## Future Improvements

- Timeout and Retransmission Support
- Multiple Client Handling
- RFC 1350 Compliance
- Better Error Packets
- Dynamic Block Size Negotiation
- IPv6 Support

---

## Learning Outcomes

This project helped in understanding:

- UDP communication
- Socket Programming & Networking
- File transfer mechanisms
- Linux networking
- Concurrency & Process Management
- Client-server protocol implementation

---

## Author

**Merin James**

B.Tech Electronics & Communication Engineering  
Embedded Systems | Linux | Networking | C Programming
