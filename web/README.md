# Remote Shutdown for Linux

Remotely schedule a shutdown or suspend for the hosting machine.

## Authentication
This authentication system uses time-based tokens to verify users. Here's how it works:

* Shared Secret:  Both the client and server possess a secret piece of information (called AUTHENTICATION_SALT). This secret is crucial for the security of the system and should never be revealed. 
* Time-Based Tokens:  Every minute, the system generates a new, unique token using a combination of the shared secret, the current time, and a special algorithm. This ensures that tokens are unpredictable and only valid for a short period. 
* Authentication Window: When a user attempts to authenticate, they present their token to the server. The server checks if the token matches the one generated for the current minute. To account for slight differences in timekeeping between the client and server, the server also checks if the token matches those generated within a small window of time around the current minute.

This approach provides a simple yet effective way to authenticate users while mitigating the risk of replay attacks (where an attacker tries to use an old, intercepted token). The use of a shared secret and constantly changing tokens makes it difficult for attackers to forge valid credentials.
## Test:
```
curl -X GET http://127.0.0.1:5000/status?token=$(python3 current_token.py)
```