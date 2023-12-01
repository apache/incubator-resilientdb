# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.    


from abc import ABCMeta, abstractmethod
from .connection import Connection
from datetime import datetime


class AbstractPicker(metaclass=ABCMeta):
    """! Abstract class for picker classes that pick connections from a pool."""

    @abstractmethod
    def pick(self, connections: list[Connection]):
        """! Picks a :class:`~resdb_driver.connection.Connection`
        instance from the given list of
        :class:`~resdb_driver.connection.Connection` instances.

        @param connections (list): List of :class:`~resdb_driver.connection.Connection` instances.
        """
        pass


class RoundRobinPicker(AbstractPicker):
    """! Picks a :class:`~resdb_driver.connection.Connection`
    instance from a list of connections.
    """

    def pick(self, connections: list[Connection]) -> Connection:
        """! Picks a connection with the earliest backoff time.
        As a result, the first connection is picked
        for as long as it has no backoff time.
        Otherwise, the connections are tried in a round robin fashion.

        @param connections (:obj:list): List of :class:`~resdb_driver.connection.Connection` instances.
        """
        if len(connections) == 1:
            return connections[0]

        return min(
            *connections,
            key=lambda conn: datetime.min
            if conn.backoff_time is None
            else conn.backoff_time
        )


class Pool:
    """! Pool of connections.
    """

    def __init__(self, connections: list[Connection], picker_class=RoundRobinPicker):
        """! Initializes a :class:`~resdb_driver.pool.Pool` instance.
        @param connections (list): List of :class:`~resdb_driver.connection.Connection` instances.
        """
        self.connections = connections
        self.picker = picker_class()

    def get_connection(self) -> Connection:
        """! Gets a :class:`~resdb_driver.connection.Connection`
        instance from the pool.
        @return A :class:`~resdb_driver.connection.Connection` instance.
        """
        return self.picker.pick(self.connections)
