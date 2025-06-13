'use client';

import React, { useEffect, useState } from 'react';
import { Box, TextInput, Switch, NumberInput, Code, Paper } from '@mantine/core';

export function PythonConfigGenerator() {
  const [config, setConfig] = useState({
    mongoUri: 'mongodb://localhost:27017',
    dbName: 'resilientdb_cache',
    collectionName: 'blocks',
    baseUrl: 'resilientdb://crow.resilientdb.com',
    httpSecure: true,
    wsSecure: true,
    reconnectInterval: 5000,
    fetchInterval: 30000,
  });

  const [generatedCode, setGeneratedCode] = useState('');

  const handleNumberChange = (
    field: 'reconnectInterval' | 'fetchInterval',
    value: number | string | undefined
  ) => {
    const numValue = typeof value === 'string' ? parseInt(value, 10) : value;
    if (numValue) {
      setConfig({ ...config, [field]: numValue });
    }
  };

  const generateCode = () => {
    const code = `from resilient_python_cache import ResilientPythonCache, MongoConfig, ResilientDBConfig

# MongoDB Configuration
mongo_config = MongoConfig(
    uri="${config.mongoUri}",
    db_name="${config.dbName}",
    collection_name="${config.collectionName}"
)

# ResilientDB Configuration
resilient_db_config = ResilientDBConfig(
    base_url="${config.baseUrl}",
    http_secure=${config.httpSecure},
    ws_secure=${config.wsSecure},
    reconnect_interval=${config.reconnectInterval},
    fetch_interval=${config.fetchInterval}
)

# Initialize cache
cache = ResilientPythonCache(mongo_config, resilient_db_config)`;

    setGeneratedCode(code);
  };

  useEffect(() => {
    generateCode();
  }, [config]);

  return (
    <Box>
      <Box mb="md">
        <TextInput
          label="MongoDB URI"
          description="Required: Connection string for MongoDB instance"
          value={config.mongoUri}
          onChange={(e) => setConfig({ ...config, mongoUri: e.target.value })}
          placeholder="mongodb://localhost:27017"
          required
        />
        <TextInput
          label="Database Name"
          description="Required: Name of the database to store blocks"
          value={config.dbName}
          onChange={(e) => setConfig({ ...config, dbName: e.target.value })}
          placeholder="resilientdb_cache"
          required
        />
        <TextInput
          label="Collection Name"
          description="Required: Name of the collection to store blocks"
          value={config.collectionName}
          onChange={(e) => setConfig({ ...config, collectionName: e.target.value })}
          placeholder="blocks"
          required
        />
        <TextInput
          label="ResilientDB Base URL"
          description="Required: URL of your ResilientDB node"
          value={config.baseUrl}
          onChange={(e) => setConfig({ ...config, baseUrl: e.target.value })}
          placeholder="resilientdb://crow.resilientdb.com"
          required
        />
        <Switch
          label="Use HTTPS"
          description="Required: Use HTTPS for REST API calls"
          checked={config.httpSecure}
          onChange={(e) => setConfig({ ...config, httpSecure: e.target.checked })}
          mt="md"
        />
        <Switch
          label="Use WSS"
          description="Required: Use WSS for WebSocket connections"
          checked={config.wsSecure}
          onChange={(e) => setConfig({ ...config, wsSecure: e.target.checked })}
          mt="xs"
        />
        <NumberInput
          label="Reconnect Interval (ms)"
          description="Optional: How often to attempt reconnection if connection is lost"
          value={config.reconnectInterval}
          onChange={(val) => handleNumberChange('reconnectInterval', val)}
          min={1000}
          max={60000}
          step={1000}
          mt="md"
          defaultValue={5000}
        />
        <NumberInput
          label="Fetch Interval (ms)"
          description="Optional: How often to sync data from ResilientDB"
          value={config.fetchInterval}
          onChange={(val) => handleNumberChange('fetchInterval', val)}
          min={5000}
          max={300000}
          step={5000}
          defaultValue={30000}
        />
      </Box>

      <Paper withBorder p="xs">
        <Code block>{generatedCode}</Code>
      </Paper>
    </Box>
  );
} 