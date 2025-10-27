/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import { invoke } from "@tauri-apps/api/core";

export const preventDefault = <T extends Event>(fn: (e: T) => void): ((e: T) => void) => {
	return (e: T) => {
		e.preventDefault();
		fn(e);
	};
};

export class GlobalState {
	private _state = $state({ name: '', greet: '' });

	get greet() {
		return this._state.greet;
	}
	set greet(value: string) {
		this._state.greet = value;
	}
	get name() {
		return this._state.name;
	}
	set name(value: string) {
		this._state.name = value;
	}
	get nlen() {
		return this.name.length;
	}

	async submit() {
		this.greet = await invoke('greet', { name: this.name });
	}

	reset() {
		this.name = '';
		this.greet = '';
	}
}
