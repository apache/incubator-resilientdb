#!/usr/bin/env python
import click
import os
import subprocess
import configparser
import bcrypt
import json
from pymongo.mongo_client import MongoClient
from pymongo.server_api import ServerApi

CONFIG_FILE_PATH = os.path.expanduser("~/.resdb_config")

config = configparser.ConfigParser()
config.read('config.ini')

MONGODB_URI = config.get('Database', 'MongoDB_URI')
DATABASE_NAME = config.get('Database', 'Name')
CURRENT_USER = config.get('User', 'Current_User')


def get_database():
    client = MongoClient(MONGODB_URI, server_api=ServerApi("1"))
    return client[DATABASE_NAME]


def get_logged_in_user():
    return CURRENT_USER


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
    # Implement login logic
    db = get_database()
    users_collection = db["users"]

    # Example: Check if the user exists
    user = users_collection.find_one({"email": username})

    if user and verify_password(password, user["password"]):
        # Login successful
        # You may want to set a session or token for the user here
        set_logged_in_user(username)
        click.echo("Login successful")
    else:
        # Login failed
        click.echo("Login failed")


@cli.command()
def logout():
    """Logout from the current account"""
    config.set('User', 'Current_User', '')
    with open('config.ini', 'w') as configfile:
        config.write(configfile)
    click.echo('Logout successful')


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

        # Store instance details in MongoDB
        db = get_database()
        instances_collection = db["instances"]

        # Check if there is an existing entry for the current user
        existing_entry = instances_collection.find_one({"user": current_user})

        if existing_entry:
            # Update the existing entry
            instances_collection.update_one(
                {"user": current_user},
                {"$set": {type: existing_entry.get(type, 0) + 1}},
            )
            click.echo(
                f"Instance created successfully. Container Name: {container_name}"
            )
        else:
            # Create a new entry for the current user
            new_entry = {"user": current_user, type: 1}
            instances_collection.insert_one(new_entry)
            click.echo(
                f"Instance created successfully. New entry created. Container Name: {container_name}"
            )
    except subprocess.CalledProcessError as error:
        click.echo(f"Error creating instance: {error}", err=True)
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

        # Remove instance details from MongoDB
        db = get_database()
        instances_collection = db["instances"]

        instances_collection.update_one(
            {"user": current_user},
            # Decrease the count by 1 (or update based on your requirements)
            {"$inc": {container_type: -1}}
        )

        click.echo("Instance deleted successfully.")
    except subprocess.CalledProcessError as error:
        click.echo(f"Error deleting instance: {error}", err=True)
    except Exception as error:
        click.echo(f"An unexpected error occurred: {error}", err=True)


if __name__ == "__main__":
    cli()
