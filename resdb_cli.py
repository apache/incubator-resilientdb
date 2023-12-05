#!/usr/bin/env python
import click
import os
import subprocess
import configparser
import bcrypt
import requests

config = configparser.ConfigParser()
config.read('config.ini')

FLASK_BASE_URL = config.get('Server', 'flask_base_url')
CURRENT_USER = config.get('User', 'Current_User')


def make_flask_request(endpoint, method, data=None):
    url = f"{FLASK_BASE_URL}/{endpoint}"
    headers = {"Content-Type": "application/json"}
    response = requests.request(method, url, json=data, headers=headers)

    if not response.ok:
        response.raise_for_status()

    return response.json()


def get_logged_in_user():
    return config.get('User', 'Current_User')


def set_logged_in_user(username):
    config.set('User', 'Current_User', username)
    with open('config.ini', 'w') as configfile:
        config.write(configfile)


def verify_password(plaintext_password, hashed_password):
    return bcrypt.checkpw(plaintext_password.encode("utf-8"), hashed_password.encode('utf-8'))


@click.group()
def cli():
    """ResDB CLI"""


@cli.command()
@click.option("--username", prompt=True)
@click.option("--password", prompt=True, hide_input=True)
def login(username, password):
    """Login to your account"""
    try:
        response = make_flask_request(
            "getUser", "POST", {"email": username, "password": password})

        if response.get("success"):
            # Login successful
            set_logged_in_user(username)
            click.echo("Login successful")
        else:
            # Login failed
            click.echo("Login failed")
    except Exception:
        click.echo(f"Incorrect username or password")


@cli.command()
def logout():
    """Logout from the current account"""
    try:
        config.set('User', 'Current_User', '')
        with open('config.ini', 'w') as configfile:
            config.write(configfile)
        click.echo('Logout successful')
    except Exception:
        click.echo('Logout failed')


@cli.command()
def whoami():
    """Display the currently logged-in user"""
    current_user = get_logged_in_user()
    if current_user:
        click.echo(f'Current user: {current_user}')
    else:
        click.echo('No user logged in.')


@cli.command()
@click.argument("type", type=click.Choice(["resdb", "sdk"]))
def create_instance(type):
    """Create a new ResDB or PythonSDK instance"""
    try:
        # Get the currently logged-in user
        current_user = get_logged_in_user()
        if not current_user:
            click.echo("No user logged in. Please log in first.")
            return

        # Implement create instance logic
        click.echo(f"Creating {type} instance for user {current_user}...")

        # Run Docker command to create a new instance
        container_name = f"{type}_instance"
        command = ["docker", "run", "--name",
                   container_name, "-d", f"expolab/{type}:arm64"]

        subprocess.run(command, check=True)

        response = make_flask_request(
            "updateInstances", "POST", {"user_email": current_user, "instance": type, "inc": True})       

        if response.get("success"):
            click.echo(
                f"Instance created successfully. Container Name: {container_name}"
            )
        else:
            click.echo("Creation failed")

    except subprocess.CalledProcessError as error:
        click.echo(f"Error creating instance: {error}", err=True)
    except Exception as error:
        click.echo(f"An unexpected error occurred: {error}", err=True)


@cli.command()
@click.argument("instance_id")
def exec_into(instance_id):
    """Bash into a running ResDB or PythonSDK instance"""
    # Implement exec logic
    try:
        # Get the currently logged-in user
        current_user = get_logged_in_user()
        if not current_user:
            click.echo("No user logged in. Please log in first.")
            return

        command = ["docker", "exec", "-it", instance_id, "bash"]
        subprocess.run(command, check=True)

    except subprocess.CalledProcessError as error:
        click.echo(f"Error executing command: {error}", err=True)
    except Exception as error:
        click.echo(f"An unexpected error occurred: {error}", err=True)


@cli.command()
def view_instances():
    """View details about running instances"""
    current_user = get_logged_in_user()
    if not current_user:
        click.echo("No user logged in. Please log in first.")
        return

    docker_command = subprocess.Popen(
        ["docker", "container", "ls", "--format", "table {{.ID}}\t{{.Image}}\t{{.Names}}"], stdout=subprocess.PIPE)

    grep_command = subprocess.Popen(
        ["grep", "sdk\|resdb"], stdin=docker_command.stdout, stdout=subprocess.PIPE)

    docker_command.stdout.close()

    output = grep_command.communicate()[0]

    click.echo(output.decode('utf-8'))


@cli.command()
@click.argument("instance_id")
def delete_instance(instance_id):
    """Delete a running ResDB or PythonSDK instance"""
    try:
        # Get the currently logged-in user
        current_user = get_logged_in_user()
        if not current_user:
            click.echo('No user logged in. Please log in first.')
            return

        # Get the container name using 'docker inspect'
        container_info = subprocess.run(["docker", "inspect", "--format", "{{.Name}}", instance_id],
                                        capture_output=True, text=True, check=True)
        container_name = container_info.stdout.strip().lstrip('/')

        if "sdk" in container_name:
            container_type = "sdk"
        else:
            container_type = "resdb"

        # Implement delete instance logic
        click.echo(f"Deleting instance {instance_id} {container_type}...")

        # Stop the Docker container
        subprocess.run(["docker", "stop", instance_id], check=True)

        # Remove the Docker container
        subprocess.run(["docker", "rm", instance_id], check=True)

        response = make_flask_request(
            "updateInstances", "POST", {"user_email": current_user, "instance": container_type, "inc": False})       

        if response.get("success"):
            click.echo(
                f"Instance deleted successfully. Container Name: {container_name}"
            )
        else:
            click.echo("Deletion failed")

        click.echo("Instance deleted successfully.")
    except subprocess.CalledProcessError as error:
        click.echo(f"Error deleting instance: {error}", err=True)
    except Exception as error:
        click.echo(f"An unexpected error occurred: {error}", err=True)


if __name__ == "__main__":
    cli()
