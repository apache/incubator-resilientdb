---
layout: default
title: "Authentication Handlers"
parent: "Requests"
nav_order: 5
---

# Chapter 5: Authentication Handlers - Showing Your ID Card

In [Chapter 4: The Cookie Jar](04_cookie_jar.md), we learned how `requests` uses `Session` objects and cookie jars to automatically remember things like login cookies. This is great for websites that use cookies to manage sessions after you log in.

But what about websites or APIs that require you to prove who you are *every time* you make a request, or use different methods than cookies? For example, some services need a username and password sent directly with the request, not just a cookie.

## The Problem: Accessing Protected Resources

Imagine a website has a special members-only area. To access pages in this area, the server needs to know you're a valid member *right when you ask for the page*. It won't just let anyone in. It needs some form of identification, like a username and password.

How do we tell `requests` to include this identification with our request?

This is where **Authentication Handlers** come in.

## What are Authentication Handlers?

Think of authentication handlers as different types of **ID badges** you can attach to your web requests. Just like you might need a specific badge to get into different parts of a building, different web services might require different types of authentication.

`Requests` has built-in support for common types (schemes) of HTTP authentication, and you can even create your own custom badges.

**Common ID Badges (Authentication Schemes):**

1.  **HTTP Basic Auth:** This is the simplest type. It's like a badge with your username and password written directly on it (encoded, but easily decoded). It's common but not very secure over plain HTTP (HTTPS makes it safer).
    *   `Requests` provides: A simple `(username, password)` tuple or the `HTTPBasicAuth` class.
2.  **HTTP Digest Auth:** This is a bit more secure than Basic. Instead of sending your password directly, it involves a challenge-response process, like the server asking a secret question based on your password, and your request providing the answer. It's more complex but avoids sending the password openly.
    *   `Requests` provides: The `HTTPDigestAuth` class.
3.  **Custom Auth:** Some services use unique authentication methods (like OAuth1, OAuth2, custom API keys).
    *   `Requests` allows you to create your own auth handlers by subclassing `AuthBase`. Many other libraries provide handlers for common schemes like OAuth.

When you provide authentication details to `requests`, it automatically figures out how to create and attach the correct `Authorization` header (or sometimes `Proxy-Authorization` for proxies) to your request. It's like pinning the right ID badge onto your request before sending it off.

## Using Authentication Handlers

The easiest way to add authentication is by using the `auth` parameter when making a request, either with the functional API or with a [Session](03_session.md) object.

### HTTP Basic Auth (The Easiest Way)

For Basic Auth, you can simply pass a tuple `(username, password)` to the `auth` argument.

Let's try accessing a test endpoint from `httpbin.org` that's protected with Basic Auth. The username is `testuser` and the password is `testpass`.

```python
import requests

# This URL requires Basic Auth with user='testuser', pass='testpass'
url = 'https://httpbin.org/basic-auth/testuser/testpass'

# Try without authentication first (should fail with 401 Unauthorized)
print("Attempting without authentication...")
response_fail = requests.get(url)
print(f"Status Code (fail): {response_fail.status_code}") # Expect 401

# Now, provide the username and password tuple to the 'auth' parameter
print("\nAttempting with Basic Auth tuple...")
try:
    response_ok = requests.get(url, auth=('testuser', 'testpass'))
    print(f"Status Code (ok): {response_ok.status_code}") # Expect 200
    # Check the response content (httpbin echoes auth info)
    print("Response JSON:")
    print(response_ok.json())
except requests.exceptions.RequestException as e:
    print(f"An error occurred: {e}")

```

**Output:**

```
Attempting without authentication...
Status Code (fail): 401

Attempting with Basic Auth tuple...
Status Code (ok): 200
Response JSON:
{'authenticated': True, 'user': 'testuser'}
```

**Explanation:**

1.  The first request failed with `401 Unauthorized` because we didn't provide credentials.
2.  In the second request, we added `auth=('testuser', 'testpass')`.
3.  `Requests` automatically recognized this tuple, created the necessary `Authorization: Basic dGVzdHVzZXI6dGVzdHBhc3M=` header (where `dGVzdHVzZXI6dGVzdHBhc3M=` is the Base64 encoding of `testuser:testpass`), and added it to the request.
4.  The server validated the credentials and granted access, returning a `200 OK` status. The response body confirms we were authenticated as `testuser`.

### Using the `HTTPBasicAuth` Class

Passing a tuple is a shortcut specifically for Basic Auth. For clarity, or if you want to reuse the authentication details, you can use the `HTTPBasicAuth` class explicitly. It does exactly the same thing internally.

```python
import requests
from requests.auth import HTTPBasicAuth # Import the class

url = 'https://httpbin.org/basic-auth/testuser/testpass'

# Create an HTTPBasicAuth object
basic_auth = HTTPBasicAuth('testuser', 'testpass')

# Pass the auth object to the 'auth' parameter
print("Attempting with HTTPBasicAuth object...")
try:
    response = requests.get(url, auth=basic_auth)
    print(f"Status Code: {response.status_code}") # Expect 200
    print("Response JSON:")
    print(response.json())
except requests.exceptions.RequestException as e:
    print(f"An error occurred: {e}")

```

**Output:**

```
Attempting with HTTPBasicAuth object...
Status Code: 200
Response JSON:
{'authenticated': True, 'user': 'testuser'}
```

This achieves the same result as the tuple, but `HTTPBasicAuth(user, pass)` is more explicit about the type of authentication being used.

### HTTP Digest Auth

Digest Auth is more complex, involving a challenge from the server. `Requests` handles this complexity for you with the `HTTPDigestAuth` class. You use it similarly to `HTTPBasicAuth`.

```python
import requests
from requests.auth import HTTPDigestAuth # Import the class

# httpbin has a digest auth endpoint
# user='testuser', pass='testpass'
url = 'https://httpbin.org/digest-auth/auth/testuser/testpass'

# Create an HTTPDigestAuth object
digest_auth = HTTPDigestAuth('testuser', 'testpass')

# Pass the auth object to the 'auth' parameter
print("Attempting with HTTPDigestAuth object...")
try:
    response = requests.get(url, auth=digest_auth)
    print(f"Status Code: {response.status_code}") # Expect 200
    print("Response JSON:")
    print(response.json())
    # Note: It might take two requests internally for Digest Auth
    print(f"Request History (if any): {response.history}")
except requests.exceptions.RequestException as e:
    print(f"An error occurred: {e}")

```

**Output:**

```
Attempting with HTTPDigestAuth object...
Status Code: 200
Response JSON:
{'authenticated': True, 'user': 'testuser'}
Request History (if any): [<Response [401]>]
```

**Explanation:**

1.  We used `HTTPDigestAuth` this time.
2.  When `requests` first tries to access the URL, the server challenges it with a `401 Unauthorized` response containing details needed for Digest Auth (like a `nonce` and `realm`). You can see this `401` response in `response.history`.
3.  The `HTTPDigestAuth` handler catches this `401`, uses the challenge information and your password to calculate the correct response, and automatically sends a *second* request with the proper `Authorization: Digest ...` header.
4.  This second request succeeds, and you get the final `200 OK` response.

`Requests` handles the two-step process automatically when you use `HTTPDigestAuth`.

### Persistent Authentication with Sessions

If you need to make multiple requests to the same server using the same authentication, it's much more efficient to set the authentication on a [Session](03_session.md) object. The session will then automatically apply the authentication to *all* requests made through it.

```python
import requests
from requests.auth import HTTPBasicAuth

basic_auth_url = 'https://httpbin.org/basic-auth/testuser/testpass'
headers_url = 'https://httpbin.org/headers' # Just to see headers sent

# Create a session
with requests.Session() as s:
    # Set the authentication ONCE on the session
    s.auth = HTTPBasicAuth('testuser', 'testpass')
    # Or: s.auth = ('testuser', 'testpass')

    # Make the first request (auth will be added automatically)
    print("Making first request using session auth...")
    response1 = s.get(basic_auth_url)
    print(f"Status Code 1: {response1.status_code}")

    # Make a second request to a different endpoint (auth will also be added)
    # We use /headers to see the Authorization header being sent
    print("\nMaking second request using session auth...")
    response2 = s.get(headers_url)
    print(f"Status Code 2: {response2.status_code}")
    print("Headers sent in second request:")
    # Look for the 'Authorization' header in the output
    print(response2.json()['headers'])
```

**Output:**

```
Making first request using session auth...
Status Code 1: 200

Making second request using session auth...
Status Code 2: 200
Headers sent in second request:
{
  "Accept": "*/*",
  "Accept-Encoding": "gzip, deflate",
  "Authorization": "Basic dGVzdHVzZXI6dGVzdHBhc3M=", // <-- Auth header added automatically!
  "Host": "httpbin.org",
  "User-Agent": "python-requests/2.x.y",
  "X-Amzn-Trace-Id": "Root=..."
}
```

By setting `s.auth = ...`, we ensured that *both* requests sent the `Authorization` header without needing to specify it in each `s.get()` call.

### Custom Authentication

What if a service uses a completely different way to authenticate? `Requests` allows you to create your own authentication handler by writing a class that inherits from `requests.auth.AuthBase` and implements the `__call__` method. This method receives the `PreparedRequest` object and should modify it (usually by adding headers) as needed.

```python
from requests.auth import AuthBase

class MyCustomApiKeyAuth(AuthBase):
    """Attaches a custom API Key header to the request."""
    def __init__(self, api_key):
        self.api_key = api_key

    def __call__(self, r):
        # 'r' is the PreparedRequest object
        # Modify the request 'r' here. We'll add a header.
        r.headers['X-API-Key'] = self.api_key
        # We MUST return the modified request object
        return r

# Usage:
# api_key = "YOUR_SECRET_API_KEY"
# response = requests.get(some_url, auth=MyCustomApiKeyAuth(api_key))
```

This is more advanced, but it shows the flexibility of the `requests` auth system. Many third-party libraries use this pattern to provide auth helpers for specific services (like OAuth).

## How It Works Internally

How does `requests` take the `auth` parameter and turn it into the correct `Authorization` header?

1.  **Preparation Step:** When you make a request (e.g., `requests.get(url, auth=...)` or `s.request(...)`), the `Request` object is turned into a `PreparedRequest` as we saw in [Chapter 2: Request & Response Models](02_request___response_models.md). Part of this preparation involves the `prepare_auth` method.
2.  **Check Auth Type:** Inside `prepare_auth`, `requests` checks the `auth` parameter.
    *   If `auth` is a tuple `(user, pass)`, it automatically wraps it in an `HTTPBasicAuth(user, pass)` object.
    *   If `auth` is already an object (like `HTTPBasicAuth`, `HTTPDigestAuth`, or a custom one inheriting from `AuthBase`), it uses that object directly.
3.  **Call the Auth Object:** All authentication handler objects (including the built-in ones) are **callable**. This means they have a `__call__` method. The `prepare_auth` step *calls* the auth object, passing the `PreparedRequest` object (`p`) to it: `auth(p)`.
4.  **Modify the Request:** The `__call__` method of the auth object does the actual work.
    *   For `HTTPBasicAuth`, the `__call__` method calculates the `Basic base64(user:pass)` string and sets `p.headers['Authorization'] = ...`.
    *   For `HTTPDigestAuth`, the `__call__` method might initially set up hooks to handle the `401` challenge, or if it already has the necessary info (like a `nonce`), it calculates the `Digest ...` header and sets `p.headers['Authorization']`.
    *   For a custom auth object, its `__call__` method performs whatever modifications are needed (e.g., adding an `X-API-Key` header).
5.  **Return Modified Request:** The `__call__` method *must* return the modified `PreparedRequest` object.
6.  **Send Request:** The `PreparedRequest`, now potentially including an `Authorization` header, is sent to the server.

Here's a simplified sequence diagram for Basic Auth:

```mermaid
sequenceDiagram
    participant UserCode as Your Code
    participant ReqFunc as requests.get / Session.request
    participant PrepReq as PreparedRequest
    participant AuthObj as HTTPBasicAuth Instance
    participant Server

    UserCode->>ReqFunc: Call get(url, auth=('user', 'pass'))
    ReqFunc->>PrepReq: Create PreparedRequest (p)
    ReqFunc->>PrepReq: Call p.prepare_auth(auth=...)
    Note over PrepReq: Detects tuple, creates HTTPBasicAuth('user', 'pass')
    PrepReq->>AuthObj: Call auth_obj(p)
    activate AuthObj
    AuthObj->>AuthObj: Calculate 'Basic ...' string
    AuthObj->>PrepReq: Set p.headers['Authorization'] = 'Basic ...'
    AuthObj-->>PrepReq: Return modified p
    deactivate AuthObj
    PrepReq-->>ReqFunc: Return prepared request p
    ReqFunc->>Server: Send HTTP Request (with Authorization header)
    Server-->>ReqFunc: Send HTTP Response
    ReqFunc-->>UserCode: Return Response
```

Let's look at the simplified code in `requests/auth.py` for `HTTPBasicAuth`:

```python
# File: requests/auth.py (Simplified)

from base64 import b64encode
from ._internal_utils import to_native_string

def _basic_auth_str(username, password):
    """Returns a Basic Auth string."""
    # ... (handle encoding username/password to bytes) ...
    auth_bytes = b":".join((username_bytes, password_bytes))
    auth_b64 = b64encode(auth_bytes).strip()
    # Return native string (str in Py3) e.g., "Basic dXNlcjpwYXNz"
    return "Basic " + to_native_string(auth_b64)

class AuthBase:
    """Base class that all auth implementations derive from"""
    def __call__(self, r):
        # This method MUST be overridden by subclasses
        raise NotImplementedError("Auth hooks must be callable.")

class HTTPBasicAuth(AuthBase):
    """Attaches HTTP Basic Authentication to the given Request object."""
    def __init__(self, username, password):
        self.username = username
        self.password = password

    def __call__(self, r):
        # 'r' is the PreparedRequest object passed in by requests
        # Calculate the Basic auth string
        auth_header_value = _basic_auth_str(self.username, self.password)
        # Modify the request's headers
        r.headers['Authorization'] = auth_header_value
        # Return the modified request
        return r

class HTTPProxyAuth(HTTPBasicAuth):
    """Attaches HTTP Proxy Authentication to a given Request object."""
    def __call__(self, r):
        # Same as Basic Auth, but sets the Proxy-Authorization header
        r.headers['Proxy-Authorization'] = _basic_auth_str(self.username, self.password)
        return r

# HTTPDigestAuth is more complex, involving state and hooks for the 401 challenge
class HTTPDigestAuth(AuthBase):
    def __init__(self, username, password):
        # ... store username/password ...
        # ... initialize state (nonce, etc.) ...
        pass

    def build_digest_header(self, method, url):
        # ... complex calculation based on nonce, realm, qop, etc. ...
        return "Digest ..." # Calculated digest header

    def handle_401(self, r, **kwargs):
        # Hook called when a 401 response is received
        # 1. Parse challenge ('WWW-Authenticate' header)
        # 2. Store nonce, realm etc.
        # 3. Prepare a *new* request with the calculated digest header
        # 4. Send the new request
        # 5. Return the response to the *new* request
        pass # Simplified

    def __call__(self, r):
        # 'r' is the PreparedRequest
        # If we already have a nonce, add the Authorization header directly
        if self.has_nonce():
            r.headers['Authorization'] = self.build_digest_header(r.method, r.url)
        # Register the handle_401 hook to handle the server challenge if needed
        r.register_hook('response', self.handle_401)
        return r
```

And in `requests/models.py`, the `PreparedRequest` calls the auth object:

```python
# File: requests/models.py (Simplified View)

from .auth import HTTPBasicAuth
from .utils import get_auth_from_url

class PreparedRequest(RequestEncodingMixin, RequestHooksMixin):
    # ... (other prepare methods like prepare_url, prepare_headers) ...

    def prepare_auth(self, auth, url=""):
        """Prepares the given HTTP auth data."""

        # If no Auth provided, maybe get it from the URL (e.g., http://user:pass@host)
        if auth is None:
            url_auth = get_auth_from_url(self.url)
            auth = url_auth if any(url_auth) else None

        if auth:
            # If auth is a ('user', 'pass') tuple, wrap it in HTTPBasicAuth
            if isinstance(auth, tuple) and len(auth) == 2:
                auth = HTTPBasicAuth(*auth)

            # --- The Core Step ---
            # Call the auth object (which must be callable, like AuthBase subclasses)
            # Pass 'self' (the PreparedRequest instance) to the auth object's __call__
            r = auth(self)

            # Update self to reflect any changes made by the auth object
            # (Auth objects typically just modify headers, but could do more)
            self.__dict__.update(r.__dict__)

            # Recompute Content-Length in case auth modified the body (unlikely for Basic/Digest)
            self.prepare_content_length(self.body)

    # ... (rest of PreparedRequest) ...
```

The key is the `r = auth(self)` line, where the `PreparedRequest` delegates the task of adding authentication details to the specific authentication handler object provided.

## Conclusion

You've learned how `requests` handles HTTP authentication using **Authentication Handlers**.

*   You saw that authentication is like providing an **ID badge** with your request.
*   You learned about common schemes like **Basic Auth** (using a simple `(user, pass)` tuple or `HTTPBasicAuth`) and **Digest Auth** (`HTTPDigestAuth`).
*   You know how to apply authentication to single requests or persistently using a [Session](03_session.md) object via the `auth` parameter.
*   You understand that internally, `requests` calls the provided auth object, which modifies the `PreparedRequest` (usually by adding an `Authorization` header) before sending it.
*   You got a glimpse of how custom authentication can be built using `AuthBase`.

Authentication is crucial for accessing protected resources. But what happens when things go wrong? A server might be down, a URL might be invalid, or authentication might fail. How does `requests` tell you about these problems?

**Next:** [Chapter 6: Exception Hierarchy](06_exception_hierarchy.md)

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)