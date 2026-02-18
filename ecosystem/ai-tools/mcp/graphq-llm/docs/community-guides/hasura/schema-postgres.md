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
description: Manage GraphQL schema over Postgres with Hasura
keywords:
  - hasura
  - docs
  - postgres
  - schema
slug: index
---

# Postgres: Schema

The Hasura GraphQL Engine automatically generates your GraphQL schema and resolvers based on your tables/views in
Postgres. **You don't need to write a GraphQL schema or resolvers.** See
[How Hasura GraphQL Engine works](/getting-started/how-it-works/index.mdx) for more details.

The Hasura Console gives you UI tools that speed up your data-modeling process, or working with your existing database.
The Console also automatically generates Migrations or Metadata files that you can edit directly and check into your
version control.

The Hasura GraphQL Engine lets you do anything you would usually do with Postgres by giving you GraphQL over native
Postgres constructs.

:::info Postgres Compatibility

Hasura works with most [Postgres compatible flavours](/databases/postgres/index.mdx#postgres-compatible-flavors).

:::

**See:**

- [Tables basics](schema/postgres/tables.mdx)
- [Table relationships](schema/postgres/table-relationships/index.mdx)
- [Remote relationships](schema/postgres/remote-relationships/index.mdx)
- [Extend with Logical Models](/schema/postgres/logical-models.mdx)
- [Extend with views](/schema/postgres/views.mdx)
- [Extend with SQL functions](/schema/postgres/custom-functions.mdx)
- [Default field values](/schema/postgres/default-values/index.mdx)
- [Enum type fields](/schema/postgres/enums.mdx)
- [Computed fields](/schema/postgres/computed-fields.mdx)
- [Customize auto-generated fields](/schema/postgres/custom-field-names.mdx)
- [Data validations](/schema/postgres/data-validations.mdx)
- [Using an existing database](/schema/postgres/using-existing-database.mdx)
- [Relay schema](/schema/postgres/relay-schema.mdx)
- [Naming convention](/schema/postgres/naming-convention.mdx)
