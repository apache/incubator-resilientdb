import json
# sample valid transaction:
# txn = {
#     "asset": {
#         "id": "ageuiagh93421941a"
#     },
#     "id": "12490124812041",
# }

# todo: take in string, convert to 
def validate(transaction):
    print(f'Validating {transaction}')

    if (not transaction) or type(transaction) is not dict:
        return False
    if "id" not in transaction:
        return False
    if "asset" not in transaction:
        return False
    if "id" not in transaction["asset"]:
        return False
    if transaction["id"] == None or transaction["asset"] == None:
        return False
    return True

txn = {
    "asset": {
        "id": "ageuiagh93421941a"
    },
    "id": "12490124812041",
}

test_str = '{"inputs": [{"owners_before": ["Gn5NZjYEUvTy92yYe41ca2jBk5NwMo48Phb313jjn1gH"], "fulfills": {"transaction_id": "067a838231600ab759bcae80e99f8f60cdc3515fa6bd9dd4cf445f79f3793005", "output_index": 0}, "fulfillment": "pGSAIOprYi3p2guMgsoEMkUlR_UdKCByqHskl0i0DX6QkdVOgUBj2-DpWnXozis7h8G4wjBiTRhRevdQfE13m_afC8XfYso5YbSWyNPdlzoyu9bx2jQhAKVjnVF86x0xBBN2_xsB"}], "outputs": [{"public_keys": ["HxA4JhEdFRK1cwXUc4pLgsTB3p42nzpzgei6xBmpNoN8"], "condition": {"details": {"type": "ed25519-sha-256", "public_key": "HxA4JhEdFRK1cwXUc4pLgsTB3p42nzpzgei6xBmpNoN8"}, "uri": "ni:///sha-256;Lo6vco_Q3hZAvqZnA83nrHU9TgBREvLfjTg5Ndq0RDs?fpt=ed25519-sha-256&cost=131072"}, "amount": "3"}, {"public_keys": ["Gn5NZjYEUvTy92yYe41ca2jBk5NwMo48Phb313jjn1gH"], "condition": {"details": {"type": "ed25519-sha-256", "public_key": "Gn5NZjYEUvTy92yYe41ca2jBk5NwMo48Phb313jjn1gH"}, "uri": "ni:///sha-256;66z4Y1OrLmJYJ9NucNAiDR6U4-OGdQdGMxw2gEB1QC4?fpt=ed25519-sha-256&cost=131072"}, "amount": "8"}], "operation": "TRANSFER", "metadata": null, "asset": {"id": "067a838231600ab759bcae80e99f8f60cdc3515fa6bd9dd4cf445f79f3793005"}, "version": "2.0", "id": "2c255fd56be65aa19cb709455352b78919b8b628f224adce370250c0f0d70fbe"}'
print("string version: " + str(test_str))
res = json.loads(test_str)
print("dict: " + str(res))

"""print(validate(4))
print(validate("string"))
print(validate(dict(id = "hi")))
print(validate(txn))"""
