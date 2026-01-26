---
layout: default
title: "Security Utilities"
parent: "FastAPI"
nav_order: 7
---

# Chapter 7: Security Utilities

Hi there! 👋 In [Chapter 6: Error Handling](06_error_handling.md), we learned how to handle situations where things go wrong in our API, like when a user requests an item that doesn't exist. Now, let's talk about protecting our API endpoints.

Imagine our online store API. Anyone should be able to browse items (`GET /items/`). But maybe only registered, logged-in users should be allowed to *create* new items (`POST /items/`) or view their own profile (`GET /users/me`). How do we ensure only the right people can access certain parts of our API?

That's where **Security Utilities** come in!

**Our Goal Today:** Learn how FastAPI provides ready-made tools to implement common security mechanisms like username/password checks or API keys, making it easy to protect your endpoints.

## What Problem Does This Solve?

When you build an API, some parts might be public, but others need protection. You need a way to:

1.  **Identify the User:** Figure out *who* is making the request. Are they logged in? Do they have a valid API key? This process is called **Authentication** (AuthN - proving who you are).
2.  **Check Permissions (Optional but related):** Once you know who the user is, you might need to check if they have permission to do what they're asking. Can user "Alice" delete user "Bob"? This is called **Authorization** (AuthZ - checking what you're allowed to do). (We'll focus mainly on Authentication in this beginner chapter).
3.  **Ask for Credentials:** How does the user provide their identity? Common ways include:
    *   **HTTP Basic Authentication:** Sending a username and password directly (encoded) in the request headers. Simple, but less secure over plain HTTP.
    *   **API Keys:** Sending a secret key (a long string) in the headers, query parameters, or cookies. Common for server-to-server communication.
    *   **OAuth2 Bearer Tokens:** Sending a temporary token (obtained after logging in) in the headers. Very common for web and mobile apps.
4.  **Document Security:** How do you tell users of your API (in the `/docs`) that certain endpoints require authentication and how to provide it?

Implementing these security schemes from scratch can be complex and tricky. FastAPI gives you pre-built components (like different types of locks and keys) that handle the common patterns for asking for and receiving credentials.

## Key Concepts

1.  **Security Schemes:** These are the standard protocols or methods used for authentication, like HTTP Basic, API Keys (in different locations), and OAuth2. FastAPI provides classes that represent these schemes (e.g., `HTTPBasic`, `APIKeyHeader`, `OAuth2PasswordBearer`). Think of these as the *type* of lock mechanism you want to install on your door.

2.  **`fastapi.security` Module:** This module contains all the pre-built security scheme classes. You'll import things like `HTTPBasic`, `APIKeyHeader`, `APIKeyQuery`, `APIKeyCookie`, `OAuth2PasswordBearer` from here.

3.  **Credentials:** The actual "secret" information the user provides to prove their identity (username/password, the API key string, the OAuth2 token string).

4.  **Verifier Dependency:** A function you write (a dependency, like we learned about in [Chapter 5: Dependency Injection](05_dependency_injection.md)) that takes the credentials extracted by the security scheme and checks if they are valid. It might check a username/password against a database or validate an API key. This function decides if the "key" fits the "lock".

5.  **`Security()` Function:** This is a special function imported from `fastapi` (`from fastapi import Security`). It works almost exactly like `Depends()`, but it's specifically designed for security dependencies. You use it like this: `user: Annotated[UserType, Security(your_verifier_dependency)]`.
    *   **Main Difference from `Depends()`:** Using `Security()` tells FastAPI to automatically add the corresponding security requirements to your OpenAPI documentation (`/docs`). This means `/docs` will show a little lock icon on protected endpoints and provide UI elements for users to enter their credentials (like username/password or a token) when trying out the API.

**Analogy:**
*   **Security Scheme (`HTTPBasic`, `APIKeyHeader`):** The type of lock on the door (e.g., a key lock, a combination lock).
*   **Scheme Instance (`security = HTTPBasic()`):** Installing that specific lock on a particular door frame.
*   **Credentials (`username/password`, `API key`):** The key or combination provided by the person trying to open the door.
*   **Verifier Dependency (`get_current_user`):** The person or mechanism that takes the key/combination, checks if it's correct, and decides whether to let the person in.
*   **`Security(get_current_user)`:** Declaring that the door requires the verifier to check the key/combination before allowing entry, and also putting a "Lock" sign on the door in the building map (`/docs`).

## Using Security Utilities: HTTP Basic Auth Example

Let's protect an endpoint using the simplest method: HTTP Basic Authentication. We'll create an endpoint `/users/me` that requires a valid username and password.

**Step 1: Import necessary tools**

We need `HTTPBasic` (the scheme), `HTTPBasicCredentials` (a Pydantic model to hold the extracted username/password), `Security` (to declare the dependency), `Annotated`, and `HTTPException` (for errors).

```python
# main.py (or your router file)
from typing import Annotated

from fastapi import Depends, FastAPI, HTTPException, status
from fastapi.security import HTTPBasic, HTTPBasicCredentials
```

**Step 2: Create an instance of the security scheme**

We create an instance of `HTTPBasic`. This object knows *how* to ask the browser/client for username/password via standard HTTP mechanisms.

```python
# Right after imports
security = HTTPBasic()

app = FastAPI() # Or use your APIRouter
```

**Step 3: Define the "Verifier" Dependency Function**

This function will receive the credentials extracted by `security` and check if they are valid. For this beginner example, we'll use hardcoded values. In a real app, you'd check against a database.

```python
# Our "verifier" function
def get_current_username(credentials: Annotated[HTTPBasicCredentials, Depends(security)]):
    # NOTE: In a real app, NEVER hardcode credentials like this!
    #       Always use secure password hashing (e.g., with passlib)
    #       and check against a database.
    correct_username = "stanley"
    correct_password = "password123" # Don't do this in production!

    # Basic check (insecure comparison for demonstration)
    is_correct_username = credentials.username == correct_username
    is_correct_password = credentials.password == correct_password # Insecure!

    if not (is_correct_username and is_correct_password):
        # If credentials are bad, raise an exception
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect email or password",
            headers={"WWW-Authenticate": "Basic"}, # Required header for 401 Basic Auth
        )
    # If credentials are okay, return the username
    return credentials.username

```

**Explanation:**

*   `get_current_username` is our dependency function.
*   `credentials: Annotated[HTTPBasicCredentials, Depends(security)]`: It depends on our `security` object (`HTTPBasic`). FastAPI will run `security` first. `security` will extract the username and password from the `Authorization: Basic ...` header and provide them as an `HTTPBasicCredentials` object to this function.
*   Inside, we perform a (very insecure, for demo only!) check against hardcoded values.
*   If the check fails, we `raise HTTPException` with status `401 Unauthorized`. The `headers={"WWW-Authenticate": "Basic"}` part is important; it tells the browser *how* it should ask for credentials (using the Basic scheme).
*   If the check passes, we return the validated username.

**Step 4: Use `Security()` in the Path Operation**

Now, let's create our protected endpoint `/users/me`. Instead of `Depends`, we use `Security` with our verifier function.

```python
@app.get("/users/me")
async def read_current_user(
    # Use Security() with the verifier function
    username: Annotated[str, Security(get_current_username)]
):
    # If the code reaches here, get_current_username ran successfully
    # and returned the validated username.
    # 'username' variable now holds the result from get_current_username.
    return {"username": username}

```

**Explanation:**

*   `username: Annotated[str, Security(get_current_username)]`: We declare that this path operation requires the `get_current_username` dependency, using `Security`.
    *   FastAPI will first run `get_current_username`.
    *   `get_current_username` will, in turn, trigger `security` (`HTTPBasic`) to get the credentials.
    *   If `get_current_username` succeeds (doesn't raise an exception), its return value (the username string) will be injected into the `username` parameter of `read_current_user`.
    *   If `get_current_username` (or the underlying `HTTPBasic`) raises an `HTTPException`, the request stops, the error response is sent, and `read_current_user` is never called.
    *   Crucially, `Security()` also adds the HTTP Basic security requirement to the OpenAPI schema for this endpoint.

**How it Behaves:**

1.  **Run the App:** `uvicorn main:app --reload`
2.  **Visit `/docs`:** Go to `http://127.0.0.1:8000/docs`.
    *   You'll see the `/users/me` endpoint now has a **padlock icon** 🔒 next to it.
    *   Click the "Authorize" button (usually near the top right). A popup will appear asking for Username and Password for the "HTTPBasic" scheme.
    *   Enter `stanley` and `password123` and click Authorize.
    *   Now, try out the `/users/me` endpoint. Click "Try it out", then "Execute". It should work and return `{"username": "stanley"}`. The browser automatically added the correct `Authorization` header because you authorized in the UI.
    *   Click "Authorize" again and "Logout". Now try executing `/users/me` again. You'll get a `401 Unauthorized` error with `{"detail": "Not authenticated"}` (this default comes from `HTTPBasic` when no credentials are provided).
3.  **Use `curl` (Command Line):**
    *   `curl http://127.0.0.1:8000/users/me` -> Returns `{"detail":"Not authenticated"}` (401).
    *   `curl -u wronguser:wrongpass http://127.0.0.1:8000/users/me` -> Returns `{"detail":"Incorrect email or password"}` (401). The `-u` flag makes `curl` use HTTP Basic Auth.
    *   `curl -u stanley:password123 http://127.0.0.1:8000/users/me` -> Returns `{"username": "stanley"}` (200 OK).

You've successfully protected an endpoint using HTTP Basic Auth!

## Other Common Schemes (Briefly)

The pattern is very similar for other schemes.

### API Key in Header

```python
# --- Imports ---
from fastapi.security import APIKeyHeader

# --- Scheme Instance ---
api_key_header_scheme = APIKeyHeader(name="X-API-KEY") # Expect key in X-API-KEY header

# --- Verifier Dependency (Example) ---
async def get_api_key(
    api_key: Annotated[str, Security(api_key_header_scheme)] # Use Security() with the SCHEME instance here
):
    if api_key == "SECRET_API_KEY": # Check the key (use a secure way in real apps!)
        return api_key
    else:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="Could not validate API KEY"
        )

# --- Path Operation ---
@app.get("/secure-data")
async def get_secure_data(
    # Inject the VALIDATED key using Depends() - no need for Security() again
    # if the get_api_key dependency already uses Security() internally.
    # Alternatively, if get_api_key just returned the key without raising errors,
    # you could use Security(get_api_key) here. Let's stick to the pattern:
    # the verifier dependency uses Security(scheme), the endpoint uses Depends(verifier)
    # or directly uses Security(verifier) if the verifier handles errors.
    # Let's adjust get_api_key to make it cleaner:
    api_key: Annotated[str, Security(api_key_header_scheme)] # Scheme extracts the key
):
    # Now, a separate check or use the key
    if api_key == "SECRET_API_KEY": # Re-checking here for simplicity, ideally done in a dependent function
         return {"data": "sensitive data", "api_key_used": api_key}
    else:
         # This path might not be reachable if auto_error=True in APIKeyHeader
         raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Invalid API Key provided")

# Let's refine the API Key example pattern to match the Basic Auth pattern:
# Scheme Instance
api_key_header_scheme = APIKeyHeader(name="X-API-KEY", auto_error=False) # auto_error=False lets verifier handle missing key

# Verifier Dependency
async def verify_api_key(api_key: Annotated[str | None, Security(api_key_header_scheme)]):
    if api_key is None:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="X-API-KEY header missing")
    if api_key == "SECRET_API_KEY":
        return api_key # Return key or user info associated with the key
    else:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Invalid API Key")

# Path Operation using the verifier
@app.get("/secure-data")
async def get_secure_data_v2(
    # Use Security() with the VERIFIER function
    verified_key: Annotated[str, Security(verify_api_key)]
):
    # verified_key holds the result from verify_api_key (the validated key)
    return {"data": "sensitive data", "key": verified_key}

```

### OAuth2 Password Bearer Flow

This is common for user logins in web apps. It usually involves two endpoints: one to exchange username/password for a token (`/token`), and protected endpoints that require the token.

```python
# --- Imports ---
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm

# --- Scheme Instance ---
# The 'tokenUrl' points to the path operation where users get the token
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")

# --- Token Endpoint (Example) ---
@app.post("/token")
async def login_for_access_token(
    form_data: Annotated[OAuth2PasswordRequestForm, Depends()]
):
    # 1. Verify form_data.username and form_data.password (check DB)
    # 2. If valid, create an access token (e.g., a JWT)
    # 3. Return the token
    # (Skipping implementation details for brevity)
    access_token = f"token_for_{form_data.username}" # Fake token
    return {"access_token": access_token, "token_type": "bearer"}

# --- Verifier Dependency (Example: decode token and get user) ---
async def get_current_user(token: Annotated[str, Security(oauth2_scheme)]):
    # In a real app:
    # 1. Decode the token (e.g., JWT)
    # 2. Validate the token (check expiry, signature)
    # 3. Extract user identifier from token payload
    # 4. Fetch user from database
    # 5. Raise HTTPException if token is invalid or user doesn't exist
    if token == "token_for_stanley": # Fake check
        return {"username": "stanley", "email": "stanley@example.com"}
    else:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid authentication credentials",
            headers={"WWW-Authenticate": "Bearer"},
        )

# --- Protected Path Operation ---
@app.get("/users/me/oauth")
async def read_users_me_oauth(
    # Use Security() with the user verifier function
    current_user: Annotated[dict, Security(get_current_user)]
):
    # current_user holds the dict returned by get_current_user
    return current_user
```

The core pattern remains: Instantiate the scheme -> Define a verifier dependency that uses the scheme -> Protect endpoints using `Security(verifier_dependency)`.

## How it Works Under the Hood (Simplified)

Let's trace the HTTP Basic Auth example (`GET /users/me` requiring `stanley`/`password123`):

1.  **Request:** Client sends `GET /users/me` with header `Authorization: Basic c3RhbmxleTpwYXNzd29yZDEyMw==` (where `c3Rh...` is base64("stanley:password123")).
2.  **Routing:** FastAPI matches the request to `read_current_user`.
3.  **Dependency Analysis:** FastAPI sees `username: Annotated[str, Security(get_current_username)]`. It knows it needs to resolve the `get_current_username` dependency using the `Security` mechanism.
4.  **Security Dependency Resolution:**
    *   FastAPI looks inside `get_current_username` and sees its dependency: `credentials: Annotated[HTTPBasicCredentials, Depends(security)]`.
    *   It needs to resolve `security` (our `HTTPBasic()` instance).
5.  **Scheme Execution (`HTTPBasic.__call__`)**:
    *   FastAPI calls the `security` object (which is callable).
    *   The `HTTPBasic` object's `__call__` method executes. It reads the `Authorization` header from the request.
    *   It finds the `Basic` scheme and the parameter `c3RhbmxleTpwYXNzd29yZDEyMw==`.
    *   It base64-decodes the parameter to get `stanley:password123`.
    *   It splits this into username (`stanley`) and password (`password123`).
    *   It creates and returns an `HTTPBasicCredentials(username="stanley", password="password123")` object.
    *   *(If the header was missing or malformed, `HTTPBasic.__call__` would raise `HTTPException(401)` here, stopping the process).*
6.  **Verifier Execution (`get_current_username`)**:
    *   FastAPI now has the result from `security`. It calls `get_current_username(credentials=<HTTPBasicCredentials object>)`.
    *   Your verifier code runs. It compares the credentials. They match the hardcoded values.
    *   The function returns the username `"stanley"`.
    *   *(If the credentials didn't match, your code would raise `HTTPException(401)` here, stopping the process).*
7.  **Path Operation Execution (`read_current_user`)**:
    *   FastAPI now has the result from `get_current_username`. It calls `read_current_user(username="stanley")`.
    *   Your path operation function runs and returns `{"username": "stanley"}`.
8.  **Response:** FastAPI sends the 200 OK JSON response back to the client.
9.  **OpenAPI Generation:** Separately, when generating `/openapi.json`, FastAPI sees `Security(get_current_username)` -> `Depends(security)` -> `security` is `HTTPBasic`. It adds the "HTTPBasic" security requirement definition to the global `components.securitySchemes` and references it in the security requirements for the `/users/me` path operation. This is what makes the lock icon appear in `/docs`.

Here's a simplified diagram:

```mermaid
sequenceDiagram
    participant Client
    participant FastAPIApp as FastAPI App
    participant HTTPBasicInst as security (HTTPBasic Instance)
    participant VerifierFunc as get_current_username
    participant PathOpFunc as read_current_user

    Client->>+FastAPIApp: GET /users/me (Authorization: Basic ...)
    FastAPIApp->>FastAPIApp: Match route, see Security(get_current_username)
    FastAPIApp->>FastAPIApp: Resolve get_current_username dependencies: Depends(security)
    FastAPIApp->>+HTTPBasicInst: Call security(request)
    HTTPBasicInst->>HTTPBasicInst: Read header, decode base64, split user/pass
    HTTPBasicInst-->>-FastAPIApp: Return HTTPBasicCredentials(user="stanley", pass="...")
    FastAPIApp->>+VerifierFunc: Call get_current_username(credentials=...)
    VerifierFunc->>VerifierFunc: Check credentials -> OK
    VerifierFunc-->>-FastAPIApp: Return username "stanley"
    FastAPIApp->>+PathOpFunc: Call read_current_user(username="stanley")
    PathOpFunc-->>-FastAPIApp: Return {"username": "stanley"}
    FastAPIApp-->>-Client: Send 200 OK JSON Response
```

## Code Connections

*   **`fastapi.Security`**: The function you import and use. It's a thin wrapper around `fastapi.params.Security`. (`fastapi/param_functions.py`)
*   **`fastapi.params.Security`**: The class that signals a security dependency, inheriting from `Depends` but adding the `scopes` parameter. (`fastapi/params.py`)
*   **`fastapi.security.*`**: This package contains the scheme implementations:
    *   `fastapi.security.http`: Contains `HTTPBase`, `HTTPBasic`, `HTTPBearer`, `HTTPDigest`, and the `HTTPBasicCredentials`, `HTTPAuthorizationCredentials` models.
    *   `fastapi.security.api_key`: Contains `APIKeyHeader`, `APIKeyQuery`, `APIKeyCookie`.
    *   `fastapi.security.oauth2`: Contains `OAuth2`, `OAuth2PasswordBearer`, `OAuth2AuthorizationCodeBearer`, `OAuth2PasswordRequestForm`, `SecurityScopes`.
*   **Scheme `__call__` methods**: Each scheme class (e.g., `HTTPBasic`, `APIKeyHeader`, `OAuth2PasswordBearer`) implements `async def __call__(self, request: Request)` which contains the logic to extract credentials from the specific request location (headers, query, etc.).
*   **Dependency Injection System**: The core system described in [Chapter 5: Dependency Injection](05_dependency_injection.md) resolves the dependencies, calling the scheme instance and then your verifier function.
*   **OpenAPI Integration**: FastAPI's OpenAPI generation logic specifically checks for `Security` dependencies and uses the associated scheme model (`security.model`) to add the correct security requirements to the schema.

## Conclusion

You've now learned the basics of securing your FastAPI endpoints!

*   You understand the need for **authentication** (who is the user?).
*   You know about common **security schemes** like HTTP Basic, API Keys, and OAuth2 Bearer tokens.
*   You learned that FastAPI provides **utility classes** (e.g., `HTTPBasic`, `APIKeyHeader`, `OAuth2PasswordBearer`) in the `fastapi.security` module to handle these schemes.
*   You saw how to use the **`Security()`** function (similar to `Depends()`) to integrate these schemes into your path operations via **verifier dependencies**.
*   You understand that `Security()` automatically adds security requirements to your **OpenAPI documentation** (`/docs`).
*   You grasped the core pattern: **Scheme Instance -> Verifier Dependency -> `Security(verifier)`**.

Using these tools allows you to easily add robust security layers to your API without reinventing the wheel.

Sometimes, after handling a request and sending a response, you might need to perform some follow-up actions, like sending a notification email or processing some data, without making the user wait. How can we do that?

Ready to run tasks in the background? Let's move on to [Chapter 8: Background Tasks](08_background_tasks.md)!

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)