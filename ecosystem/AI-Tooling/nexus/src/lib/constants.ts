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



export const TITLE_MAPPINGS: Record<string, string> = {
  'https://drive.google.com/uc?export=download&id=1Qjru0UY46IyIGnxtlVARjpQfMh3NLTmK': 'Viewstamped Replication: A New Primary Copy Method to Support Highly-Available Distributed Systems',
  'https://drive.google.com/uc?export=download&id=1Ak7BD1f0b9z-yqjnKs2jV5HZnPtFDJ8a': 'Time, Clocks, and the Ordering of Events in a Distributed System',
  'https://drive.google.com/uc?export=download&id=1HQZQWdscJ_j3kNiY2eFwC8DD4NTrwUSn': 'The Honey Badger of BFT Protocols',
  'https://drive.google.com/uc?export=download&id=14O42AFwLqhW310wnxKXJdHkDSsRNz0Ul.': 'The Byzantine Generals Problem ',
  'https://drive.google.com/uc?export=download&id=1XlTgWYCeET4k05Lccz528zx8aGO4V_KW': 'SpotLess: Concurrent Rotational Consensus Made Practical through Rapid View Synchronization',
  'https://drive.google.com/uc?export=download&id=1cLe5GDz90UlX7Ak7f9SFZXcpAl__15Yq': 'Reaching Agreement in the Presence of Faults ',
  ' https://drive.google.com/uc?export=download&id=1Hrjs8nrxZeUgxv009pQ4RDe9YEhP2qEd': 'RCC: Resilient Concurrent Consensus for High-Throughput Secure Transaction Processing',
  'https://drive.google.com/uc?export=download&id=1E_7lvwgvoAaRghUJgrWBlHkHKvQaomwS': 'Proof-of-Execution: Reaching Consensus through Fault-Tolerant Speculation',
  '  https://drive.google.com/uc?export=download&id=1xSVYUppXeByao2l35BVxvqI13Rl3C8Wu': 'Impossibility of Distributed Consensus with One Faulty Process ',
  '  https://drive.google.com/uc?export=download&id=1Lj04gXUbgyaYetn1-ruT3okdfyjSnrAE': 'Linear Consensus with One-Phase Speculation',
  '  https://drive.google.com/uc?export=download&id=1el8_j58oGbLLLepLI6rJ0h3nlEi9DH3h.': 'BFT Consensus in the Lens of Blockchain',
  '  https://drive.google.com/uc?export=download&id=1wk15nsa1ZZMMpbFjVZYpU1Ms0QfMEVZC': 'Faster Asynchronous BFT Protocols',
  'https://drive.google.com/uc?export=download&id=1BQTSvHbpUfcq9aebQns3EPPhQL_jcB4s': 'Consensus in the Presence of Partial Synchrony',
  '  https://drive.google.com/uc?export=download&id=16VMG7vFdYFpD-Ph_fU_xQXZ490MVrOWu': 'CAP Twelve Years Later: How the “Rules” Have Changed',
  '    https://drive.google.com/uc?export=download&id=1SyxiKEIyAPZ0nQZUfzXNmRB-qItBSNWj': 'Brewers Conjecture and the Feasiblity of Consistent, Available, Partitint-Tolorent Web Services',
  '   https://drive.google.com/uc?export=download&id=1u-6Pk7fyvP3g08wmev8_cc9bcN1sAMuy': 'BEAT: Asynchronous BFT Made Practical',
  ' https://drive.google.com/uc?export=download&id=1l10AfmJn9gLOT_F841123bW-Bbyg_Xfk': 'Asynchronous Secure Computations with Optimal Resilience',
  'https://drive.google.com/uc?export=download&id=1BNAH0hMIS0_84YzHGB4BLiu6OUv13Xqu': 'Asynchronous Byzantine Agreement Protocols',
  'https://drive.google.com/uc?export=download&id=1ma80WGUeShaPN-6mFCm0RT5dFaAo6BHp': 'Asymptotically Optimal Validated Asynchronous Byzantine Agreement'

};


export const TOOL_CALL_MAPPINGS: Record<string, { input: string; output: string; error: string }> = {
  "search_web": {
    input: "Searching the web...",
    output: "Searched the web",
    error: "Error searching the web",
  },
  "search_documents": {
    input: "Reading",
    output: "Read",
    error: "Error reading",
  },
}
export const MAX_TOKENS = 32000;
