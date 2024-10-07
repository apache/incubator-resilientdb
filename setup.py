# setup.py
from setuptools import setup, find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="resdb-orm",  
    version="1.0.1",   
    author="Gopal Nambiar",  
    author_email="gnambiar@ucdavis.com", 
    description="A simple ORM for ResilientDB's key-value store.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/ResilientEcosystem/ResDB-ORM",
    packages=find_packages(),
    install_requires=[
        "requests>=2.32.3",
        "PyYAML>=6.0.2",
    ],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",  
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)
