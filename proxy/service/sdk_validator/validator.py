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


#%%
from service.sdk_validator.resdb_validator.models import Transaction
from service.sdk_validator.resdb_validator.exceptions import InvalidSignature

def is_valid_tx(tx_dict: dict) -> Transaction:
    tx_obj : Transaction = Transaction.from_dict(tx_dict)

    try:
        tx_obj.validate()
        return (0, tx_obj.tx_dict)

    except InvalidSignature:
        return (1, None)



# %%

if __name__ == '__main__':
    tx_dict = {'inputs': [{'owners_before': ['9zC37hhowLSeHjknaqsZRAfakvuTR1piuin42AAvL7sG'],
                'fulfills': None,
                'fulfillment': 'pGSAIIWEFJxPvs4ClXvcd4Rnwy2h7GVmb3JME7xH5n2oBE3jgUDLke3SK_3x333Dg3Gd-1co64LWgMenHLAam3Bo48-VCjboQO0GZQJdA_5DbvgxVmoKJYc3mK7o9jiHMnb5WscL'}],
                'outputs': [{'public_keys': ['EuLjsaa21zXzf3kQ25AbwNrymdk9iMuKtrBgqkGM2uVB'],
                'condition': {'details': {'type': 'ed25519-sha-256',
                    'public_key': 'EuLjsaa21zXzf3kQ25AbwNrymdk9iMuKtrBgqkGM2uVB'},
                    'uri': 'ni:///sha-256;s9hZhqQ61Vt2jcfJSqHnPhGwtuS6zlJHRypyEoyrZfY?fpt=ed25519-sha-256&cost=131072'},
                'amount': '1'}],
                'operation': 'CREATE',
                'metadata': None,
                'asset': {'data': {'token_for': {'game_boy': {'serial_number': 'LR35902'}},
                'description': 'Time share token. Each token equals one hour of usage.'}},
                'version': '2.0',
                'id': '523db618299340e10b5c779600563285cf174aeb1362603bca66d67371584cb8'}

    ret = is_valid_tx(tx_dict)
# %%
