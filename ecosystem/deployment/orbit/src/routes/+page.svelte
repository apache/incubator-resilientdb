<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.    
-->

<script lang="ts">
	import { onMount } from 'svelte';
	import { core } from '@tauri-apps/api';
	import { getCurrentWindow } from '@tauri-apps/api/window';
  
	// shadcn or custom UI components
	import { Button } from '$lib/components/ui/button';
	import { Badge } from '$lib/components/ui/badge';
	import {
	  Table,
	  TableHeader,
	  TableBody,
	  TableRow,
	  TableHead,
	  TableCell
	} from '$lib/components/ui/table';
  
	// State
	let loading = true;
	let logMessage = "";
	let crowRunning = false;
	let graphqlRunning = false;
	let resilientdbRunning = false;
	let searchQuery = "";
  
	// For elapsed time
	let startTimestamp: number | null = null;
	let elapsedTime = "00:00:00";
  
	// SVG icons
	const playIcon = `<svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M4 3.5v9l8-4.5-8-4.5z"/></svg>`;
	const stopIcon = `<svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M4 4h8v8H4z"/></svg>`;
	const logsIcon = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 3h18v4H3z"/><path d="M3 7h18v11H3z"/><path d="M3 18h18v3H3z"/></svg>`;
  
	// Sidebar icons
	const servicesIcon = `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="2" width="20" height="14" rx="2" ry="2"/><path d="M2 22h20"/></svg>`;
	const sidebarLogsIcon = `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 4h16v16H4z"/><path d="M4 9h16"/><path d="M4 15h16"/></svg>`;
	const stopAllIcon = `<svg width="20" height="20" viewBox="0 0 16 16" fill="currentColor"><path d="M4 4h8v8H4z"/></svg>`;
  
	// Window control icons
	const minimizeIcon = `<svg width="12" height="12" viewBox="0 0 12 12" fill="currentColor"><rect y="5.5" width="12" height="1"/></svg>`;
	const maximizeIcon = `<svg width="12" height="12" viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="1"><rect x="1" y="1" width="10" height="10"/></svg>`;
	const closeIcon = `<svg width="12" height="12" viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="1"><line x1="1" y1="1" x2="11" y2="11"/><line x1="11" y1="1" x2="1" y2="11"/></svg>`;
  
	onMount(() => {
	  getCurrentWindow().setDecorations(false);
	  setupContainer();
  
	  // update elapsedTime every second
	  const interval = setInterval(() => {
		if (startTimestamp) {
		  const diff = Date.now() - startTimestamp;
		  const hrs = Math.floor(diff / 3600000);
		  const mins = Math.floor((diff % 3600000) / 60000);
		  const secs = Math.floor((diff % 60000) / 1000);
		  elapsedTime = `${String(hrs).padStart(2,'0')}:${String(mins).padStart(2,'0')}:${String(secs).padStart(2,'0')}`;
		}
	  }, 1000);
  
	  return () => clearInterval(interval);
	});
  
	async function setupContainer() {
	  try {
		logMessage = "Setting up ResilientDB environment...";
		const result = await core.invoke<string>('run_docker_container');
		logMessage = result;
		resilientdbRunning = crowRunning = graphqlRunning = true;
		startTimestamp = Date.now();
	  } catch (error) {
		logMessage = `Error: ${error}`;
	  } finally {
		loading = false;
	  }
	}
  
	async function handleStopAllServices() {
	  logMessage = "Stopping all services...";
	  const result = await core.invoke<string>('stop_docker_container');
	  logMessage = result;
	  resilientdbRunning = crowRunning = graphqlRunning = false;
	  startTimestamp = null;
	  elapsedTime = "00:00:00";
	}
  
	// Crow
	async function handleStartCrow() {
	  const r = await core.invoke<string>('start_crow');
	  logMessage = r; crowRunning = true;
	}
	async function handleStopCrow() {
	  const r = await core.invoke<string>('stop_crow');
	  logMessage = r; crowRunning = false;
	}
  
	// GraphQL
	async function handleStartGraphQL() {
	  const r = await core.invoke<string>('start_graphql');
	  logMessage = r; graphqlRunning = true;
	}
	async function handleStopGraphQL() {
	  const r = await core.invoke<string>('stop_graphql');
	  logMessage = r; graphqlRunning = false;
	}
  
	// ResilientDB (the main service)
	async function handleStartResilientDB() {
	  const r = await core.invoke<string>('start_resilientdb');
	  logMessage = r;
	  resilientdbRunning = crowRunning = graphqlRunning = true;
	  startTimestamp = Date.now();
	}
	async function handleStopResilientDB() {
	  const r = await core.invoke<string>('stop_resilientdb');
	  logMessage = r;
	  resilientdbRunning = crowRunning = graphqlRunning = false;
	  startTimestamp = null;
	  elapsedTime = "00:00:00";
	}
  
	// Logs
	const handleShowCrowLogs = () => (logMessage = "Crow logs…");
	const handleShowGraphqlLogs = () => (logMessage = "GraphQL logs…");
	const handleShowResilientdbLogs = () => (logMessage = "ResilientDB logs…");
  
	// Status Dot
	function renderStatusDot(r: boolean) {
	  return r
		? `<span class="inline-block w-2 h-2 rounded-full bg-green-500 mr-1"></span>`
		: `<span class="inline-block w-2 h-2 rounded-full bg-red-500 mr-1"></span>`;
	}
  
	// Window controls
	const onMinimize = () => getCurrentWindow().minimize();
	const onToggleMaximize = async () => {
	  const win = getCurrentWindow();
	  (await win.isMaximized()) ? win.unmaximize() : win.maximize();
	};
	const onClose = () => getCurrentWindow().close();
  </script>
  
  {#if loading}
	<div class="min-h-screen flex items-center justify-center bg-black text-neutral-300">
	  <div class="flex flex-col items-center">
		<div class="w-12 h-12 border-4 border-t-4 border-gray-600 rounded-full animate-spin"></div>
		<p class="mt-4 text-xl font-semibold">Setting up ResilientDB environment...</p>
		<p class="mt-2 text-sm">{logMessage}</p>
	  </div>
	</div>
  {:else}
	<div class="flex flex-col min-h-screen bg-black text-neutral-300">
  
	  <!-- Custom Title Bar -->
	  <header class="flex items-center px-4 py-2 bg-violet-600 shadow" data-tauri-drag-region>
		<div class="flex items-center space-x-2 select-none">
		  <div class="w-4 h-4 bg-white rounded-sm"></div>
		  <span class="font-bold text-lg">ResilientDB Orbit</span>
		</div>
		<div class="flex-1 px-4 flex justify-center">
		  <input
			type="text"
			bind:value={searchQuery}
			placeholder="Search services..."
			class="w-1/2 bg-violet-600 border border-violet-400 rounded px-2 py-1 text-neutral-50 placeholder-neutral-200 focus:outline-none focus:ring-2 focus:ring-violet-200"
		  />
		</div>
		<div class="flex space-x-2">
		  <button class="p-1 rounded hover:bg-transparent" on:click={onMinimize} title="Minimize">
			{@html minimizeIcon}
		  </button>
		  <button class="p-1 rounded hover:bg-transparent" on:click={onToggleMaximize} title="Maximize">
			{@html maximizeIcon}
		  </button>
		  <button class="p-1 rounded hover:bg-transparent" on:click={onClose} title="Close">
			{@html closeIcon}
		  </button>
		</div>
	  </header>
  
	  <div class="flex flex-1">
  
		<!-- Sidebar -->
		<aside class="w-16 bg-[#1e1e1e] border-r border-gray-700 flex flex-col items-center py-4 space-y-6">
		  <div class="p-2 rounded cursor-pointer hover:bg-transparent" title="Services">
			{@html servicesIcon}
		  </div>
		  <div class="p-2 rounded cursor-pointer hover:bg-transparent" title="Logs">
			{@html sidebarLogsIcon}
		  </div>
		  <div class="p-2 rounded cursor-pointer hover:bg-transparent" title="Stop all services" on:click={handleStopAllServices}>
			{@html stopAllIcon}
		  </div>
		</aside>
  
		<!-- Main Column -->
		<main class="flex-1 p-4 overflow-hidden">
  
		  <!-- Redesigned ResilientDB card (no border) -->
		  <div class="rounded-lg bg-[#1e1e1e] p-4 mb-4">
			<!-- Header -->
			<div class="flex items-center justify-between pb-2 mb-3 border-b border-gray-700">
			  <div class="flex items-center space-x-2">
				<h2 class="text-xl font-semibold">ResilientDB Service</h2>
				<Badge class={`select-none hover:bg-transparent ${resilientdbRunning ? 'bg-green-500 text-white' : 'bg-red-500 text-white'}`}>
				  {resilientdbRunning ? 'Running' : 'Stopped'}
				</Badge>
			  </div>
			  <Badge class="bg-violet-600 text-white select-none hover:bg-transparent">v1.0.0</Badge>
			</div>
			<!-- Body: status, elapsed time, controls -->
			<div class="space-y-4">
			  <div class="flex items-center space-x-2">
				{@html renderStatusDot(resilientdbRunning)}
				<span class="text-sm text-neutral-400">Uptime:</span>
				<span class="font-mono">{elapsedTime}</span>
			  </div>
			  <div class="flex items-center space-x-2">
				{#if !resilientdbRunning}
				  <Button class="p-2 hover:bg-transparent hover:text-current" on:click={handleStartResilientDB} title="Start ResilientDB">
					{@html playIcon} <span class="ml-1">Start</span>
				  </Button>
				{:else}
				  <Button class="p-2 hover:bg-transparent hover:text-current" on:click={handleStopResilientDB} title="Stop ResilientDB">
					{@html stopIcon} <span class="ml-1">Stop</span>
				  </Button>
				{/if}
				<Button class="p-2 hover:bg-transparent hover:text-current" on:click={handleShowResilientdbLogs} title="View Logs">
				  {@html logsIcon} <span class="ml-1">Logs</span>
				</Button>
			  </div>
			</div>
			<!-- Footer: description -->
			<div class="pt-3 mt-3 border-t border-gray-700 text-sm text-neutral-400">
			  Core database engine ensuring high availability, automatic failover, and seamless scaling for mission‑critical applications.
			</div>
		  </div>
  
		  <!-- Other services table -->
		  <div class="rounded-lg bg-[#1e1e1e] p-4 border border-gray-700">
			<Table class="w-full text-sm rounded-lg table-no-hover">
			  <TableHeader class="bg-[#2b2b2b] text-neutral-300 select-none border-b border-gray-700">
				<TableRow>
				  <TableHead class="py-3 px-4 uppercase tracking-wider">Service</TableHead>
				  <TableHead class="py-3 px-4 uppercase tracking-wider">Status</TableHead>
				  <TableHead class="py-3 px-4 uppercase tracking-wider">Actions</TableHead>
				</TableRow>
			  </TableHeader>
			  <TableBody>
				<!-- Crow -->
				<TableRow class="border-b border-gray-700">
				  <TableCell class="py-2 px-4 select-none">
					<Badge class="bg-violet-600 text-white select-none hover:bg-transparent">Crow Service</Badge>
				  </TableCell>
				  <TableCell class="py-2 px-4">
					{@html renderStatusDot(crowRunning)}{crowRunning ? 'Running' : 'Stopped'}
				  </TableCell>
				  <TableCell class="py-2 px-4">
					{#if !crowRunning}
					  <Button variant="ghost" class="p-2 mr-2 hover:bg-transparent" on:click={handleStartCrow} title="Start Crow">{@html playIcon}</Button>
					{:else}
					  <Button variant="ghost" class="p-2 mr-2 hover:bg-transparent" on:click={handleStopCrow} title="Stop Crow">{@html stopIcon}</Button>
					{/if}
					<Button variant="ghost" class="p-2 hover:bg-transparent" on:click={handleShowCrowLogs} title="View Crow Logs">{@html logsIcon}</Button>
				  </TableCell>
				</TableRow>
				<!-- GraphQL -->
				<TableRow>
				  <TableCell class="py-2 px-4 select-none">
					<Badge class="bg-violet-600 text-white select-none hover:bg-transparent">GraphQL Service</Badge>
				  </TableCell>
				  <TableCell class="py-2 px-4">
					{@html renderStatusDot(graphqlRunning)}{graphqlRunning ? 'Running' : 'Stopped'}
				  </TableCell>
				  <TableCell class="py-2 px-4">
					{#if !graphqlRunning}
					  <Button variant="ghost" class="p-2 mr-2 hover:bg-transparent" on:click={handleStartGraphQL} title="Start GraphQL">{@html playIcon}</Button>
					{:else}
					  <Button variant="ghost" class="p-2 mr-2 hover:bg-transparent" on:click={handleStopGraphQL} title="Stop GraphQL">{@html stopIcon}</Button>
					{/if}
					<Button variant="ghost" class="p-2 hover:bg-transparent" on:click={handleShowGraphqlLogs} title="View GraphQL Logs">{@html logsIcon}</Button>
				  </TableCell>
				</TableRow>
			  </TableBody>
			</Table>
		  </div>
		</main>
	  </div>
  
	  <!-- Footer -->
	  <footer class="py-2 px-4 bg-[#1e1e1e] text-sm flex items-center justify-center border-t border-gray-700">
		© {new Date().getFullYear()} Apache ResilientDB (Incubating). All rights reserved.
	  </footer>
	</div>
  {/if}
  
  <style>
	/* Hide scrollbars */
	.scrollbar-hide::-webkit-scrollbar { display: none; }
	.scrollbar-hide { -ms-overflow-style: none; scrollbar-width: none; }
  
	/* Remove row hover highlight */
	.table-no-hover tbody tr:hover,
	.table-no-hover td:hover,
	.table-no-hover th:hover {
	  background-color: transparent !important;
	}
  
	/* Lock hover colors */
	.hover\:bg-transparent:hover { background-color: transparent !important; }
	.hover\:text-current:hover { color: currentColor !important; }
  </style>
  