# The-Publish-Subscribe-Communication-Model-

The Project consists of two programs, `psclient` and `psserver`, that communicate using the publish/subscribe messaging pattern over a TCP network connection.

**psclient:**
![image](https://github.com/Mohamad11Dab/The-Publish-Subscribe-Communication-Model-/assets/114811082/3a29de0c-1527-4bbf-8c8d-5fd43d9ebda3)

- The `psclient` program is a command-line interface that allows clients to connect to the `psserver`, name themselves, subscribe to and publish topics.
- It accepts command-line arguments for the server's port number, client's name, and optional topics to subscribe to.
- After connecting to the server, it provides the client's name and sends subscription requests for the specified topics.
- It receives messages from the server and outputs them to stdout.
- It asynchronously reads lines from stdin and sends them unmodified to the server, allowing clients to publish and subscribe interactively.

**psserver:**
- The `psserver` program is the publish/subscribe server that handles multiple client connections.
- It accepts command-line arguments for the maximum number of simultaneous client connections and the port number to listen on.
- It creates a listening socket and waits for incoming connections from clients.
- Upon receiving a client connection, it spawns a new thread to handle that client.
- The server allows clients to name themselves, subscribe and unsubscribe from topics, and publish messages with values for topics.
- The communication between the clients and the server is done using a simple command protocol over TCP.

**Communication Protocol:**
- The protocol includes commands like 'name', 'subscribe', 'unsubscribe', and 'publish'.
- Clients send commands to the server, and the server processes them accordingly.
- The server sends publication notices to clients when topics they are subscribed to get published by other clients.

**Requirements and Error Handling:**
- Both `psclient` and `psserver` should perform error checking on command-line arguments and handle invalid inputs.
- `psclient` should handle stdin inputs from users and send them to the server without any error checking.
- The programs should terminate gracefully when appropriate (e.g., upon receiving EOF, network errors, or reaching the maximum client connections).
- The interaction between clients and the server should be asynchronous and non-blocking.
- The programs should not use signal handlers or attempt to mask or block signals.

**Example Usage:**
- Clients can subscribe to topics and receive publication notices from the server.
- They can publish messages with values for topics.
- Clients can unsubscribe from topics to stop receiving publication notices for those topics.

It's a simple publish/subscribe system with clients and a server, providing robust error handling and a responsive communication system. The specifics of the communication protocol and threading implementation are crucial to achieving the required functionality.
