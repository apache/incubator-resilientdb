# NexresGraphQL
Nexres GraphQL Server

# Start
`python app.py`

OR

(If you're using this for the first time, the steps below would be useful)

Installation steps for Windows 21H2 ecosystem:

1.  Install Ubuntu 20.04 from the Microsoft Windows store

2. Once installed, go to File Explorer -> Linux -> Ubuntu 20.04

3. In the home directory, once the password is configured, copy/clone this directory

4. Install python3 (version - 3.10.7) and ensure pip is installed using the command

`sudo apt-get install python3-pip`

5. Also make sure to install the venv module which creates a virtual Python environment that helps encapsulate the project's dependencies and prevents possible conflicts with the global Python environment. The command is:

`sudo apt-get install -y python3-venv`

6. To start a virtual env:

`source venv/bin/activate`

7. (Temporary resolve) Copy app.py to the nexres_sdk folder to ensure execution

8. (Optional) If there is an error while running any command relating to NexRes, this command below may help:

`sudo apt-get install python3.10-distutils`

Note: If there is an error with the pip version, use the command:
`curl -sS https://bootstrap.pypa.io/get-pip.py | python3.10`

9. Open a venv in the Ubuntu server and run the command: 

`source venv/bin/activate`
`sudo apt-get install cloud-init`

`deactivate` would take you back to the Ubuntu environment

10. Install the remaining dependencies in venv

`python3 -m pip install strawberry-graphql`
`python3 -m pip install flask_cors`

11. Run the requirements.txt command to install the SDK related dependencies

`python3 -m pip install -r requirements.txt`

Finally, start the app using:

`python3 app.py`

This would be hosted on your local server to test the React App/Frontend.
Also, this would give info on the console about the POST command, which are used to create an account. This consists of 3 stages: publicKey. AES encrypted private key and decrypted privateKey

Note: To run Ubuntu virtual instance, open Command Prompt, then type `ubuntu2004` to open an instance