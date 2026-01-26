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

// @typescript-eslint/restrict-template-expressions
import ReactDOM from 'react-dom';
import React from 'react';
import Box from '@webapp/ui/Box';
import { decodeFlamebearer } from '@webapp/models/flamebearer';
import { FlamegraphRenderer } from '@pyroscope/flamegraph/src/FlamegraphRenderer';
import Footer from './components/Footer';
import '@pyroscope/flamegraph/dist/index.css';

if (!(window as ShamefulAny).flamegraph) {
  alert(`'flamegraph' is required`);
  throw new Error(`'flamegraph' is required`);
}

// TODO parse window.flamegraph
const { flamegraph } = window as ShamefulAny;

function StandaloneApp() {
  const flamebearer = decodeFlamebearer(flamegraph);

  return (
    <div>
      <Box>
        <FlamegraphRenderer
          flamebearer={flamebearer as ShamefulAny}
          ExportData={null}
        />
      </Box>
      <Footer />
    </div>
  );
}

function run() {
  ReactDOM.render(<StandaloneApp />, document.getElementById('root'));
}

// Since InlineChunkHtmlPlugin adds scripts to the head
// We must wait for the DOM to be loaded
// Otherwise React will fail to initialize since there's no DOM yet
window.addEventListener('DOMContentLoaded', run, false);
