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

# 🎓 Example Prompts for GraphQL Query Tutor

Use these example prompts to test the GraphQL Query Tutor in Nexus. Copy and paste them into the query editor.

---

## 📊 GraphQL Query Examples (For Analysis)

### **Basic Queries**

#### 1. Simple Transaction Query
```graphql
{
  getTransaction(id: "123") {
    id
    asset
  }
}
```

#### 2. Minimal Query
```graphql
{
  getTransaction(id: "test") {
    id
  }
}
```

#### 3. Transaction with Selected Fields
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

### **Complete Queries**

#### 4. Full Transaction Details
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

#### 5. Named Query
```graphql
query GetTransaction {
  getTransaction(id: "test123") {
    id
    asset
    amount
  }
}
```

#### 6. Query with Aliases
```graphql
query GetTransactionDetails {
  transaction: getTransaction(id: "alias_test") {
    transactionId: id
    transactionType: type
    transactionOperation: operation
    transactionAmount: amount
    assetData: asset
  }
}
```

### **Mutation Examples**

#### 7. Create Transaction Mutation
```graphql
mutation CreateAsset {
  postTransaction(
    data: {
      operation: "CREATE"
      amount: 100
      signerPublicKey: "public_key_string"
      signerPrivateKey: "private_key_string"
      recipientPublicKey: "recipient_public_key"
      asset: {
        "data": {
          "name": "My Asset",
          "description": "Asset description"
        }
      }
    }
  ) {
    id
  }
}
```

#### 8. Simple Mutation
```graphql
mutation {
  postTransaction(
    data: {
      operation: "CREATE"
      amount: 1
      signerPublicKey: "key1"
      signerPrivateKey: "key2"
      recipientPublicKey: "key3"
      asset: { "data": { "test": "value" } }
    }
  ) {
    id
  }
}
```

---

## ❓ Question-Based Prompts (For RAG)

### **GraphQL Basics**

1. **What is GraphQL?**
2. **How do I write a GraphQL query?**
3. **What is the difference between query and mutation?**
4. **How do I use variables in GraphQL queries?**
5. **What are GraphQL fragments and how do I use them?**
6. **How does GraphQL introspection work?**
7. **What are GraphQL directives?**

### **ResilientDB Specific**

8. **What is ResilientDB?**
9. **How do I retrieve a transaction by ID in GraphQL?**
10. **What fields are available in the getTransaction query?**
11. **How do I create a transaction using GraphQL?**
12. **What is the PrepareAsset input type?**
13. **What operations are available in ResilientDB?**
14. **How do I transfer an asset using GraphQL?**

### **Query Optimization**

15. **How can I optimize my GraphQL queries for better performance?**
16. **What fields should I select to improve query efficiency?**
17. **How do I reduce the complexity of my GraphQL queries?**
18. **What are best practices for writing GraphQL queries?**

### **Advanced Topics**

19. **How do I filter transactions in GraphQL?**
20. **How does GraphQL pagination work?**
21. **How do I handle errors in GraphQL queries?**
22. **What is the JSONScalar type in GraphQL?**
23. **How do I query nested data in GraphQL?**

### **Schema Exploration**

24. **What types and fields are available in the GraphQL schema?**
25. **What mutations are available in the GraphQL schema?**
26. **How do I explore the GraphQL schema?**
27. **What is the structure of a RetrieveTransaction type?**

---

## 🎯 Recommended Testing Order

### **Phase 1: Start Simple**
1. Try **Query #1** (simple transaction query)
2. Ask **Question #1**: "What is GraphQL?"
3. Try **Query #2** (minimal query)
4. Ask **Question #9**: "How do I retrieve a transaction by ID in GraphQL?"

### **Phase 2: Intermediate**
5. Try **Query #4** (full transaction details)
6. Ask **Question #15**: "How can I optimize my GraphQL queries?"
7. Try **Query #5** (named query)
8. Ask **Question #11**: "How do I create a transaction using GraphQL?"

### **Phase 3: Advanced**
9. Try **Query #6** (aliases)
10. Try **Mutation #7** (create transaction)
11. Ask **Question #20**: "How does GraphQL pagination work?"
12. Ask **Question #24**: "What types and fields are available in the GraphQL schema?"

### **Phase 4: Edge Cases**
13. Try **Query #3** (selected fields - test optimization suggestions)
14. Ask **Question #22**: "What is the JSONScalar type in GraphQL?"
15. Try **Mutation #8** (simple mutation)

---

## 📋 Quick Copy-Paste List

### **Queries (Copy these into the query editor):**

1. `{ getTransaction(id: "123") { id asset } }`
2. `{ getTransaction(id: "test") { id } }`
3. `{ getTransaction(id: "txn_001") { id operation amount asset } }`
4. `{ getTransaction(id: "abc123") { id version amount uri type publicKey signerPublicKey operation metadata asset } }`
5. `query GetTransaction { getTransaction(id: "test123") { id asset amount } }`
6. `query GetTransactionDetails { transaction: getTransaction(id: "alias_test") { transactionId: id transactionType: type transactionOperation: operation transactionAmount: amount assetData: asset } }`
7. `mutation CreateAsset { postTransaction(data: { operation: "CREATE" amount: 100 signerPublicKey: "key1" signerPrivateKey: "key2" recipientPublicKey: "key3" asset: { "data": { "name": "Test" } } }) { id } }`

### **Questions (Copy these into the query editor):**

1. What is GraphQL?
2. How do I write a GraphQL query?
3. What is the difference between query and mutation?
4. How do I retrieve a transaction by ID in GraphQL?
5. How do I create a transaction using GraphQL?
6. How can I optimize my GraphQL queries for better performance?
7. What fields are available in the getTransaction query?
8. What is ResilientDB?
9. How does GraphQL pagination work?
10. What is the JSONScalar type in GraphQL?

---

## 💡 What to Check

### **For GraphQL Queries:**
- ✅ **Explanation Tab:** Should explain what the query does, its purpose, and how it works
- ✅ **Optimization Tab:** Should provide suggestions for improving the query (field selection, complexity reduction, etc.)
- ✅ **Efficiency Tab:** Should show:
  - Efficiency Score (0-100)
  - Estimated Execution Time
  - Resource Usage (CPU, Memory, Network)
  - Complexity Level (Low/Medium/High)

### **For Questions:**
- ✅ **Explanation Tab:** Should provide a clear, comprehensive answer using RAG-retrieved documentation
- ✅ **Documentation References:** Should cite relevant GraphQL/ResilientDB documentation
- ✅ **Examples:** Should include code examples when relevant
- ✅ **Completeness:** Should fully answer the question

---

## 🔍 Expected Results

### **Simple Queries (1, 2, 3):**
- **Complexity:** Low
- **Efficiency Score:** 90-100
- **Optimization Suggestions:** May suggest removing unused fields or using field aliases

### **Complete Queries (4):**
- **Complexity:** Medium/High
- **Efficiency Score:** 70-90
- **Optimization Suggestions:** Should suggest selecting only needed fields

### **Mutations (7, 8):**
- **Complexity:** Medium
- **Efficiency Score:** 80-95
- **Optimization Suggestions:** May suggest validating input data or optimizing asset structure

### **Questions:**
- Should retrieve 3-5 relevant documentation chunks
- Should provide comprehensive, well-structured answers
- Should include examples when helpful
- Should reference ResilientDB/GraphQL documentation

---

## 🚀 Pro Tips

1. **Mix Queries and Questions:** Alternate between actual GraphQL queries and natural language questions
2. **Check All Tabs:** Review Explanation, Optimization, and Efficiency tabs for each input
3. **Compare Similar Queries:** Try variations of the same query to see how suggestions differ
4. **Test Edge Cases:** Try invalid queries or very complex queries to see error handling
5. **Verify Documentation:** Ensure explanations reference the correct documentation chunks

---

**Happy Testing! 🎓**

