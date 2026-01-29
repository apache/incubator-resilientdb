<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

# 🧪 Test Queries for GraphQL Tutor

Use these 10 queries to test the GraphQL Tutor in Nexus. Copy and paste them into the query editor.

---

## 1. **Basic Transaction Query (Simple)**
**Complexity:** Low  
**Purpose:** Test basic query retrieval

```graphql
{
  getTransaction(id: "123") {
    id
    asset
  }
}
```

---

## 2. **Full Transaction Details (Complete)**
**Complexity:** Medium  
**Purpose:** Test retrieving all available fields

```graphql
{
  getTransaction(id: "abc123") {
    id
    version
    amount
    uri
    type
    publicKey
    signerPublicKey
    operation
    metadata
    asset
  }
}
```

---

## 3. **Transaction with Selected Fields (Focused)**
**Complexity:** Low  
**Purpose:** Test field selection optimization

```graphql
{
  getTransaction(id: "txn_001") {
    id
    operation
    amount
    asset
  }
}
```

---

## 4. **Transaction Metadata Query (Specific)**
**Complexity:** Low  
**Purpose:** Test querying specific transaction metadata

```graphql
{
  getTransaction(id: "metadata_test_123") {
    id
    version
    metadata
    operation
  }
}
```

---

## 5. **Transaction with Asset Details (Nested)**
**Complexity:** Medium  
**Purpose:** Test JSON scalar handling

```graphql
{
  getTransaction(id: "asset_456") {
    id
    asset
    type
    operation
  }
}
```

---

## 6. **Transaction Key Fields Only (Minimal)**
**Complexity:** Low  
**Purpose:** Test minimal field selection

```graphql
{
  getTransaction(id: "minimal_789") {
    id
    operation
  }
}
```

---

## 7. **Transaction with Public Keys (Security Context)**
**Complexity:** Medium  
**Purpose:** Test security-related fields

```graphql
{
  getTransaction(id: "key_test_001") {
    id
    publicKey
    signerPublicKey
    operation
    type
  }
}
```

---

## 8. **Transaction Amount and URI (Financial)**
**Complexity:** Low  
**Purpose:** Test financial transaction fields

```graphql
{
  getTransaction(id: "amount_uri_123") {
    id
    amount
    uri
    version
  }
}
```

---

## 9. **Complete Transaction with All Metadata (Comprehensive)**
**Complexity:** High  
**Purpose:** Test comprehensive field retrieval

```graphql
query GetFullTransaction {
  getTransaction(id: "comprehensive_001") {
    id
    version
    amount
    uri
    type
    publicKey
    signerPublicKey
    operation
    metadata
    asset
  }
}
```

---

## 10. **Transaction Query with Alias (Advanced)**
**Complexity:** Medium  
**Purpose:** Test GraphQL aliases and query naming

```graphql
query GetTransactionDetails {
  transaction: getTransaction(id: "alias_test_123") {
    transactionId: id
    transactionType: type
    transactionOperation: operation
    transactionAmount: amount
    assetData: asset
  }
}
```

---

## 📝 Mutation Examples (For Testing Mutations)

### **Create Transaction Mutation**

```graphql
mutation CreateTransaction {
  postTransaction(
    data: {
      operation: "CREATE"
      amount: 100
      signerPublicKey: "signer_pub_key_123"
      signerPrivateKey: "signer_priv_key_123"
      recipientPublicKey: "recipient_pub_key_123"
      asset: {
        "name": "Test Asset"
        "description": "This is a test asset"
        "value": 100
      }
    }
  ) {
    id
  }
}
```

### **Transfer Transaction Mutation**

```graphql
mutation TransferTransaction {
  postTransaction(
    data: {
      operation: "TRANSFER"
      amount: 50
      signerPublicKey: "signer_pub_key_456"
      signerPrivateKey: "signer_priv_key_456"
      recipientPublicKey: "recipient_pub_key_456"
      asset: {
        "from": "sender_id"
        "to": "recipient_id"
        "amount": 50
      }
    }
  ) {
    id
  }
}
```

---

## 🎯 Testing Strategy

### **Progressive Testing:**
1. Start with **Query #1** (simplest) to verify basic functionality
2. Move to **Query #2** (complete) to test all fields
3. Try **Query #9** (comprehensive) to test complex queries
4. Test **Query #10** (aliases) to verify advanced GraphQL features

### **What to Check:**
- ✅ **Explanation:** Should explain what the query does
- ✅ **Complexity:** Should show complexity level (low/medium/high)
- ✅ **Documentation Context:** Should reference relevant docs from `docs/`
- ✅ **Optimization Suggestions:** Should provide actionable recommendations
- ✅ **Efficiency Metrics:** Should show estimated performance

### **Expected Behaviors:**
- Simple queries (1, 3, 6) should show **low complexity**
- Complete queries (2, 9) should show **medium/high complexity**
- Queries with many fields should suggest **field selection optimization**
- All queries should include **documentation context** from ResilientDB docs

---

## 💡 Tips

1. **Use Real Transaction IDs:** Replace placeholder IDs with actual transaction IDs from your ResilientDB
2. **Test Edge Cases:** Try queries with non-existent IDs to test error handling
3. **Compare Results:** Run the same query multiple times to see consistency
4. **Check Documentation:** Verify that explanations reference the correct documentation
5. **Test Mutations:** Use mutation examples to test write operations (if supported)

---

## 🔍 Query Analysis Focus Areas

When testing, pay attention to:

1. **Field Selection:** Does the tutor suggest selecting only needed fields?
2. **Query Complexity:** Is the complexity assessment accurate?
3. **Documentation Relevance:** Are the retrieved docs relevant to the query?
4. **Optimization Suggestions:** Are suggestions actionable and helpful?
5. **Efficiency Predictions:** Are performance estimates reasonable?

---

**Happy Testing! 🚀**

