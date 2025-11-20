# 🧪 Nexus Test Inputs - Mix of Queries and Questions

Use these inputs to test the GraphQL Tutor in Nexus. Copy and paste them into the query editor.

---

## 📊 GraphQL Queries (10 queries)

### **1. Simple Transaction Query**
```graphql
{ getTransaction(id: "123") { asset } }
```

### **2. Complete Transaction with All Fields**
```graphql
{ getTransaction(id: "abc123") { id version amount uri type publicKey signerPublicKey operation metadata asset } }
```

### **3. Transaction with Selected Fields**
```graphql
{ getTransaction(id: "txn_001") { id operation amount asset } }
```

### **4. Transaction Metadata Query**
```graphql
{ getTransaction(id: "metadata_test") { id version metadata operation } }
```

### **5. Transaction with Named Query**
```graphql
query GetTransaction { getTransaction(id: "test123") { id asset amount } }
```

### **6. Minimal Transaction Query**
```graphql
{ getTransaction(id: "minimal") { id operation } }
```

### **7. Transaction with Public Keys**
```graphql
{ getTransaction(id: "key_test") { id publicKey signerPublicKey operation type } }
```

### **8. Transaction Amount and URI**
```graphql
{ getTransaction(id: "amount_uri") { id amount uri version } }
```

### **9. Complete Transaction Query**
```graphql
query GetFullTransaction { getTransaction(id: "comprehensive") { id version amount uri type publicKey signerPublicKey operation metadata asset } }
```

### **10. Transaction with Aliases**
```graphql
query GetTransactionDetails { transaction: getTransaction(id: "alias_test") { transactionId: id transactionType: type transactionOperation: operation transactionAmount: amount assetData: asset } }
```

---

## ❓ General Questions & Doubts (15 questions)

### **GraphQL Basics:**
1. **What is GraphQL?**
2. **How do I write a GraphQL query?**
3. **What is the difference between query and mutation?**
4. **How do I use variables in GraphQL?**
5. **What are GraphQL fragments?**

### **ResilientDB Specific:**
6. **What is ResilientDB?**
7. **How do I retrieve a transaction by ID?**
8. **What fields are available in the Query type?**
9. **How do I create a transaction using GraphQL?**
10. **What is the PrepareAsset input type?**

### **GraphQL Advanced Topics:**
11. **How do I filter transactions?**
12. **What mutations are available?**
13. **How does GraphQL pagination work?**
14. **What is GraphQL introspection?**
15. **How do I handle errors in GraphQL?**

---

## 🎯 Recommended Testing Order

### **Phase 1: Basic Queries (Start Here)**
1. Start with **Query #1** (simplest)
2. Try **Question #1**: "What is GraphQL?"
3. Try **Query #3** (selected fields)
4. Try **Question #2**: "How do I write a GraphQL query?"

### **Phase 2: Intermediate Testing**
5. Try **Query #5** (named query)
6. Try **Question #7**: "How do I retrieve a transaction by ID?"
7. Try **Query #2** (complete transaction)
8. Try **Question #9**: "How do I create a transaction using GraphQL?"

### **Phase 3: Advanced Testing**
9. Try **Query #9** (comprehensive)
10. Try **Query #10** (aliases)
11. Try **Question #11**: "How do I filter transactions?"
12. Try **Question #13**: "How does GraphQL pagination work?"

### **Phase 4: Edge Cases**
13. Try **Question #6**: "What is ResilientDB?"
14. Try **Query #6** (minimal)
15. Try **Question #15**: "How do I handle errors in GraphQL?"

---

## 📝 What to Check for Each Input

### **For GraphQL Queries:**
- ✅ **Explanation:** Should explain what the query does
- ✅ **Complexity:** Should show complexity level (low/medium/high)
- ✅ **Documentation Context:** Should reference relevant docs
- ✅ **Optimization Suggestions:** Should provide actionable recommendations
- ✅ **Efficiency Metrics:** Should show estimated performance (score, time, resources)

### **For General Questions:**
- ✅ **Answer Quality:** Should provide clear, helpful answers
- ✅ **Documentation Usage:** Should use RAG to retrieve relevant docs
- ✅ **Context Awareness:** Should understand the question
- ✅ **Examples:** Should include examples when helpful
- ✅ **Completeness:** Should answer the question fully

---

## 💡 Testing Tips

1. **Mix It Up:** Alternate between queries and questions
2. **Compare Results:** Try similar queries/questions to see consistency
3. **Check All Tabs:** Review Explanation, Optimization, and Efficiency tabs
4. **Verify Documentation:** Ensure explanations reference correct docs
5. **Test Edge Cases:** Try invalid queries or unclear questions
6. **Check Response Time:** Note how long each analysis takes

---

## 🔍 Expected Behaviors

### **Simple Queries (1, 3, 6):**
- Complexity: **Low**
- Efficiency Score: **90-100**
- Should suggest: Field selection optimization (if many fields)

### **Complete Queries (2, 9):**
- Complexity: **Medium/High**
- Efficiency Score: **70-90**
- Should suggest: Select only needed fields

### **General Questions:**
- Should retrieve 3-5 relevant documentation chunks
- Should provide comprehensive answers
- Should include examples when relevant
- Should reference ResilientDB/GraphQL documentation

---

## 📋 Quick Copy-Paste List

**Copy these one by one into Nexus:**

**Queries:**
1. `{ getTransaction(id: "123") { asset } }`
2. `{ getTransaction(id: "abc123") { id version amount uri type publicKey signerPublicKey operation metadata asset } }`
3. `{ getTransaction(id: "txn_001") { id operation amount asset } }`
4. `{ getTransaction(id: "metadata_test") { id version metadata operation } }`
5. `query GetTransaction { getTransaction(id: "test123") { id asset amount } }`

**Questions:**
1. What is GraphQL?
2. How do I write a GraphQL query?
3. What is ResilientDB?
4. How do I retrieve a transaction by ID?
5. How do I create a transaction using GraphQL?

---

**Happy Testing! 🚀**

