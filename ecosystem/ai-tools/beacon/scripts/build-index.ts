#!/usr/bin/env tsx

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

import { documentIndexer } from '../lib/documentIndexer';

async function buildIndex() {
  try {
    console.log('🚀 Starting document index build...');
    
    const index = await documentIndexer.indexDocuments();
    
    console.log('✅ Document index built successfully!');
    console.log(`📊 Indexed ${index.totalDocuments} document chunks`);
    console.log(`📅 Last updated: ${index.lastUpdated}`);
    console.log(`💾 Index saved to: public/document-index.json`);
    
  } catch (error) {
    console.error('❌ Error building document index:', error);
    process.exit(1);
  }
}

buildIndex();
