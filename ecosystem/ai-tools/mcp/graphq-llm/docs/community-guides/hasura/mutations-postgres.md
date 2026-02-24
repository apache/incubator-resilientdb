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

---
description: Manage mutations on Postgres with Hasura
keywords:
  - hasura
  - docs
  - postgres
  - mutation
slug: index
---

# Postgres: GraphQL Mutations

## Introduction

GraphQL mutations are used to modify data on the server (i.e. write, update or delete data).

Hasura GraphQL Engine auto-generates mutations as part of the GraphQL schema from your Postgres schema model.

Data of all tables in the database tracked by the GraphQL Engine can be modified over the GraphQL endpoint. If you have
a tracked table in your database, its insert/update/delete mutation fields are added as nested fields under the
`mutation_root` root level type.

:::info Postgres Compatibility

Hasura works with most [Postgres compatible flavours](/databases/postgres/index.mdx#postgres-compatible-flavors).

:::

## Types of mutation requests

The following types of mutation requests are possible:

- [Insert](/mutations/postgres/insert.mdx)
- [Upsert](/mutations/postgres/upsert.mdx)
- [Update](/mutations/postgres/update.mdx)
- [Delete](/mutations/postgres/delete.mdx)
- [Multiple mutations in a request](/mutations/postgres/multiple-mutations.mdx)

## Other configuration

By default, Hasura treats unset nullable variables with no default as being
nulls. However, according to the GraphQL specification, these values should be
removed from the query. Thus, with the experimental feature flag
[`no_null_unbound_variable_default`](/deployment/graphql-engine-flags/reference.mdx#experimental-features),
users can opt into this behavior.
