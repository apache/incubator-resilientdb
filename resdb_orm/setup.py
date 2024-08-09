# setup.py
from setuptools import setup, find_packages

setup(
    name="resdb_orm",
    version="0.1",
    packages=find_packages(),
    install_requires=[
        "requests",
    ],
    description="A simple ORM for ResilientDB's key-value store.",
    author="Your Name",
)
