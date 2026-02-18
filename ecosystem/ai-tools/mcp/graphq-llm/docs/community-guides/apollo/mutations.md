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

# Apollo – Mutations

> Source: [https://www.apollographql.com/docs/react/data/mutations/](https://www.apollographql.com/docs/react/data/mutations/)
> Retrieved: 2025-11-19T07:02:42.934383Z

[ Docs  ](/docs)

[ ](/docs)

Search Apollo content (Cmd+K or /)

[Launch GraphOS Studio ](https://studio.apollographql.com/signup?referrer=docs)

[ Home Home ](/docs/)[ Schema Design Schema Design ](/docs/graphos/schema-design)[ Connectors Connectors ](/docs/graphos/connectors)[ GraphOS Platform GraphOS Platform ](/docs/graphos/platform)[ Routing Routing ](/docs/graphos/routing)[ Resources Resources ](/docs/graphos/resources)

Apollo Client (Web) Apollo Client (Web)

[Rover CLI](https://www.apollographql.com/docs/rover)[IDE Support](https://www.apollographql.com/docs/ide-support)[Apollo Client (Web)](https://www.apollographql.com/docs/react)[Apollo iOS](https://www.apollographql.com/docs/ios)[Apollo Kotlin](https://www.apollographql.com/docs/kotlin)[Apollo Server](https://www.apollographql.com/docs/apollo-server)[Apollo MCP Server](https://www.apollographql.com/docs/apollo-mcp-server)[Apollo Operator](https://www.apollographql.com/docs/apollo-operator)

Apollo Client (Web) - v4 (latest)

[Introduction](https://www.apollographql.com/docs/react)[Why Apollo Client?](https://www.apollographql.com/docs/react/why-apollo)[Get started](https://www.apollographql.com/docs/react/get-started)

### Core concepts

[Queries](https://www.apollographql.com/docs/react/data/queries)[Suspense](https://www.apollographql.com/docs/react/data/suspense)[Fragments](https://www.apollographql.com/docs/react/data/fragments)[Mutations](https://www.apollographql.com/docs/react/data/mutations)[TypeScript](https://www.apollographql.com/docs/react/data/typescript)[Refetching](https://www.apollographql.com/docs/react/data/refetching)[Subscriptions](https://www.apollographql.com/docs/react/data/subscriptions)[Directives](https://www.apollographql.com/docs/react/data/directives)[Error handling](https://www.apollographql.com/docs/react/data/error-handling)[Persisted queries](https://www.apollographql.com/docs/react/data/persisted-queries)[Document transforms](https://www.apollographql.com/docs/react/data/document-transforms)[Deferring response data](https://www.apollographql.com/docs/react/data/defer)[Best practices](https://www.apollographql.com/docs/react/data/operation-best-practices)

### Caching

[Overview](https://www.apollographql.com/docs/react/caching/overview)[Configuration](https://www.apollographql.com/docs/react/caching/cache-configuration)[Reading and writing](https://www.apollographql.com/docs/react/caching/cache-interaction)[Garbage collection and eviction](https://www.apollographql.com/docs/react/caching/garbage-collection)[Customizing field behavior](https://www.apollographql.com/docs/react/caching/cache-field-behavior)[Memory Management](https://www.apollographql.com/docs/react/caching/memory-management)[Advanced topics](https://www.apollographql.com/docs/react/caching/advanced-topics)

### Pagination

[Overview](https://www.apollographql.com/docs/react/pagination/overview)[Core API](https://www.apollographql.com/docs/react/pagination/core-api)[Offset-based](https://www.apollographql.com/docs/react/pagination/offset-based)[Cursor-based](https://www.apollographql.com/docs/react/pagination/cursor-based)[keyArgs](https://www.apollographql.com/docs/react/pagination/key-args)

### Local State

[Overview](https://www.apollographql.com/docs/react/local-state/local-state-management)[Local-only fields](https://www.apollographql.com/docs/react/local-state/managing-state-with-field-policies)[Reactive variables](https://www.apollographql.com/docs/react/local-state/reactive-variables)[Local resolvers](https://www.apollographql.com/docs/react/local-state/local-resolvers)

### Development & Testing

[Developer tools](https://www.apollographql.com/docs/react/development-testing/developer-tooling)[GraphQL Codegen](https://www.apollographql.com/docs/react/development-testing/graphql-codegen)[Testing React components](https://www.apollographql.com/docs/react/development-testing/testing)[Schema-driven testing](https://www.apollographql.com/docs/react/development-testing/schema-driven-testing)[Mocking schema capabilities](https://www.apollographql.com/docs/react/development-testing/client-schema-mocking)[Reducing bundle size](https://www.apollographql.com/docs/react/development-testing/reducing-bundle-size)

### Performance

[Improving performance](https://www.apollographql.com/docs/react/performance/performance)[Optimistic mutation results](https://www.apollographql.com/docs/react/performance/optimistic-ui)[Server-side rendering](https://www.apollographql.com/docs/react/performance/server-side-rendering)[Compiling queries with Babel](https://www.apollographql.com/docs/react/performance/babel)

### Integrations

[Using Apollo Client with your view layer](https://www.apollographql.com/docs/react/integrations/integrations)[Integrating with React Native](https://www.apollographql.com/docs/react/integrations/react-native)[Loading queries with Webpack](https://www.apollographql.com/docs/react/integrations/webpack)

### Networking

[Basic HTTP networking](https://www.apollographql.com/docs/react/networking/basic-http-networking)[Advanced HTTP networking](https://www.apollographql.com/docs/react/networking/advanced-http-networking)[Authentication](https://www.apollographql.com/docs/react/networking/authentication)

### API Reference

### Core

[ApolloClient](https://www.apollographql.com/docs/react/api/core/ApolloClient)[InMemoryCache](https://www.apollographql.com/docs/react/api/cache/InMemoryCache)[ObservableQuery](https://www.apollographql.com/docs/react/api/core/ObservableQuery)

### Errors

[CombinedGraphQLErrors](https://www.apollographql.com/docs/react/api/errors/CombinedGraphQLErrors)[CombinedProtocolErrors](https://www.apollographql.com/docs/react/api/errors/CombinedProtocolErrors)[LinkError](https://www.apollographql.com/docs/react/api/errors/LinkError)[LocalStateError](https://www.apollographql.com/docs/react/api/errors/LocalStateError)[ServerError](https://www.apollographql.com/docs/react/api/errors/ServerError)[ServerParseError](https://www.apollographql.com/docs/react/api/errors/ServerParseError)[UnconventionalError](https://www.apollographql.com/docs/react/api/errors/UnconventionalError)

### React

[ApolloProvider](https://www.apollographql.com/docs/react/api/react/ApolloProvider)[useQuery](https://www.apollographql.com/docs/react/api/react/useQuery)[useLazyQuery](https://www.apollographql.com/docs/react/api/react/useLazyQuery)[useMutation](https://www.apollographql.com/docs/react/api/react/useMutation)[useSubscription](https://www.apollographql.com/docs/react/api/react/useSubscription)[useFragment](https://www.apollographql.com/docs/react/api/react/useFragment)[useApolloClient](https://www.apollographql.com/docs/react/api/react/useApolloClient)[useReactiveVar](https://www.apollographql.com/docs/react/api/react/useReactiveVar)[useSuspenseQuery](https://www.apollographql.com/docs/react/api/react/useSuspenseQuery)[useBackgroundQuery](https://www.apollographql.com/docs/react/api/react/useBackgroundQuery)[useReadQuery](https://www.apollographql.com/docs/react/api/react/useReadQuery)[useLoadableQuery](https://www.apollographql.com/docs/react/api/react/useLoadableQuery)[useQueryRefHandlers](https://www.apollographql.com/docs/react/api/react/useQueryRefHandlers)[skipToken](https://www.apollographql.com/docs/react/api/react/skipToken)[createQueryPreloader](https://www.apollographql.com/docs/react/api/react/preloading)[MockProvider](https://www.apollographql.com/docs/react/api/react/testing)[SSR](https://www.apollographql.com/docs/react/api/react/ssr)

### Apollo Link

[Overview](https://www.apollographql.com/docs/react/api/link/introduction)[ApolloLink](https://www.apollographql.com/docs/react/api/link/apollo-link)[BaseHttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-base-http)[BaseBatchHttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-base-batch-http)[BatchHttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-batch-http)[BatchLink](https://www.apollographql.com/docs/react/api/link/apollo-link-batch)[ClientAwarenessLink](https://www.apollographql.com/docs/react/api/link/apollo-link-client-awareness)[ErrorLink](https://www.apollographql.com/docs/react/api/link/apollo-link-error)[GraphQLWsLink](https://www.apollographql.com/docs/react/api/link/apollo-link-subscriptions)[HttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-http)[PersistedQueryLink](https://www.apollographql.com/docs/react/api/link/apollo-link-persisted-queries)[RemoveTypenameFromVariablesLink](https://www.apollographql.com/docs/react/api/link/apollo-link-remove-typename)[RetryLink](https://www.apollographql.com/docs/react/api/link/apollo-link-retry)[SchemaLink](https://www.apollographql.com/docs/react/api/link/apollo-link-schema)[SetContextLink](https://www.apollographql.com/docs/react/api/link/apollo-link-context)[WebSocketLink (deprecated)](https://www.apollographql.com/docs/react/api/link/apollo-link-ws)[Community links](https://www.apollographql.com/docs/react/api/link/community-links)

[Changelog](https://github.com/apollographql/apollo-client/blob/main/CHANGELOG.md)[Migrating to Apollo Client 4.0](https://www.apollographql.com/docs/react/migrating/apollo-client-4-migration)[Versioning Policy](https://www.apollographql.com/docs/react/versioning-policy)

Navigation controls

## Schema Design

[Overview](https://www.apollographql.com/docs/graphos/schema-design)

### Federated Schemas

[Overview](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/federation)[Schema Types](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/schema-types)[Schema Composition](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/composition)[Sharing Types (Value Types)](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/sharing-types)

### Federation Reference

[Federation Changelog](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/versions)[Federation Directives](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/directives)[Composition Rules](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/composition-rules)

### Subgraph Reference

[Federation Subgraphs Specification](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/subgraph-spec)[Subgraph Specific Fields](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/subgraph-specific-fields)[Compatible Subgraph Libraries](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/compatible-subgraphs)

### Development and Tooling

[Query Plans](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/query-plans)[Composition Hints](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/hints)[Errors](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/errors)

### From Federation 1 to 2

[Upgrade Guide](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/moving-to-federation-2)[Federation 2 Backwards Compatibility](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/reference/backward-compatibility)

### Entities

[Introduction](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/intro)

### Entity Essentials

[Entity Fields](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/contribute-fields)[Advanced Keys](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/define-keys)[Entity Interfaces](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/interfaces)

### Entity Guides

[Entity Best Practices](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/best-practices)[Migrate Fields](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/migrate-fields)[Resolve Another Subgraph's Fields](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/resolve-another-subgraphs-fields)[Enforce Ownership](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/enforce-ownership)[Use Contexts](https://www.apollographql.com/docs/graphos/schema-design/federated-schemas/entities/use-contexts)

### Schema Design Guides

### Schema Design Principles

[Demand Oriented Schema Design](https://www.apollographql.com/docs/graphos/schema-design/guides/demand-oriented-schema-design)[Naming Conventions](https://www.apollographql.com/docs/graphos/schema-design/guides/naming-conventions)[Namespacing](https://www.apollographql.com/docs/graphos/schema-design/guides/namespacing-by-separation-of-concerns)

### Server-Driven UI

[Basics](https://www.apollographql.com/docs/graphos/schema-design/guides/sdui/basics)[Schema Design](https://www.apollographql.com/docs/graphos/schema-design/guides/sdui/schema-design)[Client Design](https://www.apollographql.com/docs/graphos/schema-design/guides/sdui/client-design)

### Data Handling Techniques

[Handling the N+1 Problem](https://www.apollographql.com/docs/graphos/schema-design/guides/handling-n-plus-one)[Nullability](https://www.apollographql.com/docs/graphos/schema-design/guides/nullability)[Interfaces](https://www.apollographql.com/docs/graphos/schema-design/guides/interfaces)[Response Type Pattern](https://www.apollographql.com/docs/graphos/schema-design/guides/response-type-pattern)[Errors as Data](https://www.apollographql.com/docs/graphos/schema-design/guides/errors-as-data-explained)[Aggregating Data](https://www.apollographql.com/docs/graphos/schema-design/guides/aggregating-data-across-subgraphs)[Graph Identities](https://www.apollographql.com/docs/graphos/schema-design/guides/graph-identities)

### Schema Development and Evolution

[Mocking to Unblock Development](https://www.apollographql.com/docs/graphos/schema-design/guides/mocking)[Deprecations](https://www.apollographql.com/docs/graphos/schema-design/guides/deprecations)[Distributed Orchestration](https://www.apollographql.com/docs/graphos/schema-design/guides/distributed-orchestration)[Migrating from a Monolith](https://www.apollographql.com/docs/graphos/schema-design/guides/from-monolith)[Migrating from Schema Stitching](https://www.apollographql.com/docs/graphos/schema-design/guides/migrating-from-stitching)

## Connectors

[Overview](https://www.apollographql.com/docs/graphos/connectors)[Why use Connectors?](https://www.apollographql.com/docs/graphos/connectors/why-connectors)

### Getting Started

[Quickstart](https://www.apollographql.com/docs/graphos/connectors/getting-started)[API Requirements](https://www.apollographql.com/docs/graphos/connectors/getting-started/requirements)[Version Requirements](https://www.apollographql.com/docs/graphos/connectors/getting-started/version-requirements)

### Connectors Library

[Overview](https://www.apollographql.com/docs/graphos/connectors/library)

### Prebuilt Connectors

[Anthropic](https://www.apollographql.com/docs/graphos/connectors/library/anthropic)[Open AI](https://www.apollographql.com/docs/graphos/connectors/library/openai)[AWS DynamoDB](https://www.apollographql.com/docs/graphos/connectors/library/aws-dynamodb)[AWS Lambda](https://www.apollographql.com/docs/graphos/connectors/library/aws-lambda)[Strapi](https://www.apollographql.com/docs/graphos/connectors/library/strapi)[Stripe](https://www.apollographql.com/docs/graphos/connectors/library/stripe)[OData](https://www.apollographql.com/docs/graphos/connectors/library/odata)

### Development and Tooling

[Overview](https://www.apollographql.com/docs/graphos/connectors/tooling)[IDE Extensions](https://www.apollographql.com/docs/graphos/connectors/tooling/ide-extensions)[Using Rover](https://www.apollographql.com/docs/graphos/connectors/tooling/rover)[CLI Tools](https://www.apollographql.com/docs/graphos/connectors/tooling/cli-tools)[Mapping Playground](https://www.apollographql.com/docs/graphos/connectors/tooling/mapping-playground)

### Making Requests

[Overview](https://www.apollographql.com/docs/graphos/connectors/requests)[Request URLs](https://www.apollographql.com/docs/graphos/connectors/requests/url)[Request Headers](https://www.apollographql.com/docs/graphos/connectors/requests/headers)[Request Bodies](https://www.apollographql.com/docs/graphos/connectors/requests/body)[Batch Requests](https://www.apollographql.com/docs/graphos/connectors/requests/batching)[gRPC over JSON](https://www.apollographql.com/docs/graphos/connectors/requests/grpc-json-bridge)

### Handling Responses

[Overview](https://www.apollographql.com/docs/graphos/connectors/responses)[Mapping Response Fields](https://www.apollographql.com/docs/graphos/connectors/responses/fields)[Error Handling](https://www.apollographql.com/docs/graphos/connectors/responses/error-handling)

### Working with Entities

[Overview](https://www.apollographql.com/docs/graphos/connectors/entities)[Common Patterns](https://www.apollographql.com/docs/graphos/connectors/entities/patterns)[Working Across Subgraphs](https://www.apollographql.com/docs/graphos/connectors/entities/across-subgraphs)

### Mapping Language

[Overview](https://www.apollographql.com/docs/graphos/connectors/mapping)[Handling Arrays](https://www.apollographql.com/docs/graphos/connectors/mapping/arrays)[Mapping Enums](https://www.apollographql.com/docs/graphos/connectors/mapping/enums)[Using Literal Values](https://www.apollographql.com/docs/graphos/connectors/mapping/literals)[Variable Reference](https://www.apollographql.com/docs/graphos/connectors/mapping/variables)[Method Reference](https://www.apollographql.com/docs/graphos/connectors/mapping/methods)

### Deployment

[Overview](https://www.apollographql.com/docs/graphos/connectors/deployment)[Configuration](https://www.apollographql.com/docs/graphos/connectors/deployment/configuration)

### Deployment Configurations

[Overriding Base URLs](https://www.apollographql.com/docs/graphos/connectors/deployment/overriding-base-urls)[Overriding Headers](https://www.apollographql.com/docs/graphos/connectors/deployment/overriding-headers)

### Security Configurations

[Overview](https://www.apollographql.com/docs/graphos/connectors/security)[Authentication](https://www.apollographql.com/docs/graphos/connectors/security/auth)[Request Limits](https://www.apollographql.com/docs/graphos/connectors/security/request-limits)[TLS](https://www.apollographql.com/docs/graphos/connectors/security/tls)

### Performance Configurations

[Overview](https://www.apollographql.com/docs/graphos/connectors/performance)[Traffic Shaping](https://www.apollographql.com/docs/graphos/connectors/performance/traffic-shaping)

### Observability

[Overview](https://www.apollographql.com/docs/graphos/connectors/observability)[Telemetry Configurations](https://www.apollographql.com/docs/graphos/connectors/observability/telemetry)

### Testing

[Testing Framework](https://www.apollographql.com/docs/graphos/connectors/testing)

### Connectors Reference

[Changelog](https://www.apollographql.com/docs/graphos/connectors/reference/changelog)[Directives](https://www.apollographql.com/docs/graphos/connectors/reference/directives)[Limitations](https://www.apollographql.com/docs/graphos/connectors/reference/limitations)

[Troubleshooting](https://www.apollographql.com/docs/graphos/connectors/troubleshooting)

## GraphOS Platform

[Overview](https://www.apollographql.com/docs/graphos/platform)[Apollo Sandbox](https://www.apollographql.com/docs/graphos/platform/sandbox)

### Schema Management

[Overview](https://www.apollographql.com/docs/graphos/platform/schema-management)

### Schema Delivery

### Publishing

[Overview](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/publishing)[Publish with Rover](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/publishing/rover)[Publish with the Platform API](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/publishing/platform-api)[Guide: Publish with GitHub Actions](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/publishing/guide-github)

[Launches](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/launch)[Graph Artifacts](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/graph-artifacts)

### Contracts

[Overview](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/contracts/overview)[Create](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/contracts/create)[Usage Patterns](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/contracts/usage-patterns)[Reference](https://www.apollographql.com/docs/graphos/platform/schema-management/delivery/contracts/reference)

### Schema Checks

[Overview](https://www.apollographql.com/docs/graphos/platform/schema-management/checks)[Run Checks](https://www.apollographql.com/docs/graphos/platform/schema-management/checks/run)[Custom Checks](https://www.apollographql.com/docs/graphos/platform/schema-management/checks/custom)[Checks Configuration](https://www.apollographql.com/docs/graphos/platform/schema-management/checks/configure)[GitHub Integration](https://www.apollographql.com/docs/graphos/platform/schema-management/checks/github-integration)[Checks Reference](https://www.apollographql.com/docs/graphos/platform/schema-management/checks/reference)

### Schema Linting

[Overview](https://www.apollographql.com/docs/graphos/platform/schema-management/linting)[Linter Rules](https://www.apollographql.com/docs/graphos/platform/schema-management/linting/rules)

### Schema Proposals

[Overview](https://www.apollographql.com/docs/graphos/platform/schema-management/proposals)[Configure](https://www.apollographql.com/docs/graphos/platform/schema-management/proposals/configure)[Create](https://www.apollographql.com/docs/graphos/platform/schema-management/proposals/create)[Review](https://www.apollographql.com/docs/graphos/platform/schema-management/proposals/review)[Implement](https://www.apollographql.com/docs/graphos/platform/schema-management/proposals/implement)

### Explorer IDE

[Overview](https://www.apollographql.com/docs/graphos/platform/explorer)[Connect and Authenticate](https://www.apollographql.com/docs/graphos/platform/explorer/connect)[Operation Collections](https://www.apollographql.com/docs/graphos/platform/explorer/operation-collections)[Subscription Support](https://www.apollographql.com/docs/graphos/platform/explorer/subscription-support)[Embed](https://www.apollographql.com/docs/graphos/platform/explorer/embed)[Scripting](https://www.apollographql.com/docs/graphos/platform/explorer/scripting)[Additional Features](https://www.apollographql.com/docs/graphos/platform/explorer/additional-features)

### Graph Security

[Overview](https://www.apollographql.com/docs/graphos/platform/security/overview)[Persisted Queries](https://www.apollographql.com/docs/graphos/platform/security/persisted-queries)

### Metrics and Insights

[Overview](https://www.apollographql.com/docs/graphos/platform/insights)

### Metrics Collection and Forwarding

[Sending Metrics to GraphOS](https://www.apollographql.com/docs/graphos/platform/insights/sending-operation-metrics)[Operation Signatures](https://www.apollographql.com/docs/graphos/platform/insights/operation-signatures)[Datadog Forwarding](https://www.apollographql.com/docs/graphos/platform/insights/datadog-forwarding)

### Insights and Analysis

[Operation Metrics](https://www.apollographql.com/docs/graphos/platform/insights/operation-metrics)[Field Usage](https://www.apollographql.com/docs/graphos/platform/insights/field-usage)[Error Diagnostics](https://www.apollographql.com/docs/graphos/platform/insights/errors)[Subgraph Metrics](https://www.apollographql.com/docs/graphos/platform/insights/subgraphs)[Segmenting by Client](https://www.apollographql.com/docs/graphos/platform/insights/client-segmentation)

### Notifications and Alerts

[Overview](https://www.apollographql.com/docs/graphos/platform/insights/notifications)[Daily Reports](https://www.apollographql.com/docs/graphos/platform/insights/notifications/daily-reports)[Schema Changes](https://www.apollographql.com/docs/graphos/platform/insights/notifications/schema-changes)[Schema Proposals](https://www.apollographql.com/docs/graphos/platform/insights/notifications/schema-proposals)[Performance Alerts](https://www.apollographql.com/docs/graphos/platform/insights/notifications/performance-alerts)[Build Status](https://www.apollographql.com/docs/graphos/platform/insights/notifications/build-status)

### Access Management

[Organizations](https://www.apollographql.com/docs/graphos/platform/access-management/org)[API Keys](https://www.apollographql.com/docs/graphos/platform/access-management/api-keys)[Members, Roles, and Permissions](https://www.apollographql.com/docs/graphos/platform/access-management/member-roles)[Audit Logs](https://www.apollographql.com/docs/graphos/platform/access-management/audit-log)

### SSO

[Overview](https://www.apollographql.com/docs/graphos/platform/access-management/sso/overview)[Multi-Organization](https://www.apollographql.com/docs/graphos/platform/access-management/sso/multi-organization)

### SAML

[Okta](https://www.apollographql.com/docs/graphos/platform/access-management/sso/saml-okta)[Microsoft Entra ID](https://www.apollographql.com/docs/graphos/platform/access-management/sso/saml-microsoft-entra-id)[Generic SAML Setup](https://www.apollographql.com/docs/graphos/platform/access-management/sso/saml-integration-guide)

### OIDC

[Okta](https://www.apollographql.com/docs/graphos/platform/access-management/sso/oidc-okta)[Microsoft Entra ID](https://www.apollographql.com/docs/graphos/platform/access-management/sso/oidc-microsoft-entra-id)[Generic OIDC Setup](https://www.apollographql.com/docs/graphos/platform/access-management/sso/oidc-integration-guide)

### SCIM

[Overview](https://www.apollographql.com/docs/graphos/platform/access-management/scim)[Okta](https://www.apollographql.com/docs/graphos/platform/access-management/scim/okta)[Microsoft Entra ID](https://www.apollographql.com/docs/graphos/platform/access-management/scim/microsoft-entra-id)

### Graph Management

[Using Variants](https://www.apollographql.com/docs/graphos/platform/graph-management/variants)[Adding Subgraphs](https://www.apollographql.com/docs/graphos/platform/graph-management/add-subgraphs)[Managing Subgraphs](https://www.apollographql.com/docs/graphos/platform/graph-management/manage-subgraphs)[Updating Graph Components](https://www.apollographql.com/docs/graphos/platform/graph-management/updates)[Transferring Graphs](https://www.apollographql.com/docs/graphos/platform/graph-management/transfers)

### Production Readiness

[Production Readiness Checklist](https://www.apollographql.com/docs/graphos/platform/production-readiness/checklist)

### Testing and Load Management

[Overload Protection](https://www.apollographql.com/docs/graphos/platform/production-readiness/overload-protection)[Load Testing](https://www.apollographql.com/docs/graphos/platform/production-readiness/load-testing)[Testing with Federation](https://www.apollographql.com/docs/graphos/platform/production-readiness/testing-with-apollo-federation)

### Environment and Deployment

[Environment Best Practices](https://www.apollographql.com/docs/graphos/platform/production-readiness/environment-best-practices)[Schema Change Management](https://www.apollographql.com/docs/graphos/platform/production-readiness/change-management)[Updating Client Schema](https://www.apollographql.com/docs/graphos/platform/production-readiness/updating-client-schema)[Deployment Best Practices](https://www.apollographql.com/docs/graphos/platform/production-readiness/deployment-best-practices)

[Platform Limits](https://www.apollographql.com/docs/graphos/platform/platform-limits)[GraphOS Platform API](https://www.apollographql.com/docs/graphos/platform/platform-api)[GraphOS MCP Tools](https://www.apollographql.com/docs/graphos/platform/graphos-mcp-tools)

## Routing

Apollo Router - v2 (latest)

[Overview](https://www.apollographql.com/docs/graphos/routing)[Get Started](https://www.apollographql.com/docs/graphos/routing/get-started)[Request Lifecycle](https://www.apollographql.com/docs/graphos/routing/request-lifecycle)

### Configuration

[Overview](https://www.apollographql.com/docs/graphos/routing/configuration/overview)[Environment Variable Reference](https://www.apollographql.com/docs/graphos/routing/configuration/envvars)[CLI Reference](https://www.apollographql.com/docs/graphos/routing/configuration/cli)[YAML Reference](https://www.apollographql.com/docs/graphos/routing/configuration/yaml)

### Features

### Security

[Overview](https://www.apollographql.com/docs/graphos/routing/security)[Persisted Queries](https://www.apollographql.com/docs/graphos/routing/security/persisted-queries)[Authorization](https://www.apollographql.com/docs/graphos/routing/security/authorization)

### Authentication

[JWT Authentication](https://www.apollographql.com/docs/graphos/routing/security/jwt)[Router Authentication](https://www.apollographql.com/docs/graphos/routing/security/router-authentication)[Subgraph Authentication](https://www.apollographql.com/docs/graphos/routing/security/subgraph-authentication)

[CORS](https://www.apollographql.com/docs/graphos/routing/security/cors)[CSRF Prevention](https://www.apollographql.com/docs/graphos/routing/security/csrf)[TLS](https://www.apollographql.com/docs/graphos/routing/security/tls)[Request Limits](https://www.apollographql.com/docs/graphos/routing/security/request-limits)[Demand Control](https://www.apollographql.com/docs/graphos/routing/security/demand-control)

### Observability

### Studio Insights

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/graphos)[Operation Metrics Reporting](https://www.apollographql.com/docs/graphos/routing/observability/graphos/graphos-reporting)[Federated Trace Data](https://www.apollographql.com/docs/graphos/routing/observability/graphos/federated-trace-data)

### Router Telemetry (OTEL)

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel)

### Telemetry Data

### Metrics

[Instruments](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/instruments)[Standard Instruments](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/standard-instruments)

[Events](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/events)

### Traces

[Spans](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/spans)[Standard Attributes](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/standard-attributes)

[Conditions](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/conditions)[Selectors](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/selectors)

### Usage Guides

### Subgraph Observability

[Subgraph Error Inclusion](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/usage-guides/subgraph-error-inclusion)[Debugging Subgraph Requests](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/usage-guides/debugging-subgraph-requests)[Instrumenting Subgraphs](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/usage-guides/subgraph-instrumentation)

### Client Observability

[Debugging Client Requests](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/usage-guides/debugging-client-requests)[Client ID Enforcement](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/enabling-telemetry/usage-guides/client-id-enforcement)

### Telemetry Exporters

### Metrics Exporters

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/telemetry-pipelines/metrics-exporters/overview)[OTLP](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/telemetry-pipelines/metrics-exporters/otlp)

### Log Exporters

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/telemetry-pipelines/log-exporters/overview)[Stdout](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/telemetry-pipelines/log-exporters/stdout)

### Trace Exporters

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/telemetry-pipelines/trace-exporters/overview)[OTLP](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/telemetry-pipelines/trace-exporters/otlp)

### APM Guides

### Datadog

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog)

### Connecting to Datadog

[Overview](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog/connecting-to-datadog)[OpenTelemetry Collector](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog/connecting-to-datadog/otel-collector)

### Datadog Agent

[Metrics](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog/connecting-to-datadog/datadog-agent/datadog-agent-metrics)[Traces](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog/connecting-to-datadog/datadog-agent/datadog-agent-traces)

[Router Instrumentation](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog/router-instrumentation)[Dashboard Template](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/datadog/observing-and-monitoring/dashboard-template)

### New Relic

[Metrics](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/new-relic/new-relic-otlp-metrics)[Traces](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/new-relic/new-relic-otlp-traces)

### Prometheus

[Metrics](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/prometheus/prometheus-metrics)[Trace Metrics](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/prometheus/otel-traces-to-prometheus)

### Zipkin

[Traces](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/zipkin/zipkin-traces)

### Jaeger

[Traces](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/jaeger/jaeger-traces)

### Dynatrace

[Metrics](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/dynatrace/dynatrace-metrics)[Traces](https://www.apollographql.com/docs/graphos/routing/observability/router-telemetry-otel/apm-guides/dynatrace/dynatrace-traces)

### Performance and Scaling

[Overview](https://www.apollographql.com/docs/graphos/routing/performance)

### Response Caching

[Overview](https://www.apollographql.com/docs/graphos/routing/performance/caching/response-caching/overview)[Quickstart](https://www.apollographql.com/docs/graphos/routing/performance/caching/response-caching/quickstart)[Invalidation](https://www.apollographql.com/docs/graphos/routing/performance/caching/response-caching/invalidation)[Customization](https://www.apollographql.com/docs/graphos/routing/performance/caching/response-caching/customization)[Observability](https://www.apollographql.com/docs/graphos/routing/performance/caching/response-caching/observability)[FAQ](https://www.apollographql.com/docs/graphos/routing/performance/caching/response-caching/faq)

[Traffic Shaping](https://www.apollographql.com/docs/graphos/routing/performance/traffic-shaping)[Query Batching](https://www.apollographql.com/docs/graphos/routing/performance/query-batching)

### Client Features

[APQ](https://www.apollographql.com/docs/graphos/routing/operations/apq)[HTTP Header Propagation](https://www.apollographql.com/docs/graphos/routing/header-propagation)[@defer](https://www.apollographql.com/docs/graphos/routing/operations/defer)

### GraphQL Subscriptions

[Overview](https://www.apollographql.com/docs/graphos/routing/operations/subscriptions/overview)[Configuration](https://www.apollographql.com/docs/graphos/routing/operations/subscriptions/configuration)[Callback Protocol](https://www.apollographql.com/docs/graphos/routing/operations/subscriptions/callback-protocol)[Multipart Protocol](https://www.apollographql.com/docs/graphos/routing/operations/subscriptions/multipart-protocol)[Enable with API Gateway](https://www.apollographql.com/docs/graphos/routing/operations/subscriptions/api-gateway)

[File Upload](https://www.apollographql.com/docs/graphos/routing/operations/file-upload)

### Query Planning

[Native Query Planner](https://www.apollographql.com/docs/graphos/routing/query-planning/native-query-planner)[Best Practices](https://www.apollographql.com/docs/graphos/routing/query-planning/query-planning-best-practices)[Caching](https://www.apollographql.com/docs/graphos/routing/query-planning/caching)

### Customization

[Overview](https://www.apollographql.com/docs/graphos/routing/customization/overview)

### Coprocessors

[Coprocessor Configuration](https://www.apollographql.com/docs/graphos/routing/customization/coprocessor)[Coprocessor Reference](https://www.apollographql.com/docs/graphos/routing/customization/coprocessor/reference)

### Rhai Scripts

[Rhai Configuration](https://www.apollographql.com/docs/graphos/routing/customization/rhai)[Rhai API Reference](https://www.apollographql.com/docs/graphos/routing/customization/rhai/reference)

### Custom Builds

[Building a Binary](https://www.apollographql.com/docs/graphos/routing/customization/custom-binary)[Rust Plugins](https://www.apollographql.com/docs/graphos/routing/customization/native-plugins)

### Deployment

[Overview](https://www.apollographql.com/docs/graphos/routing/self-hosted)

### Docker

[Docker with the Apollo Runtime Container](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/docker)[Docker with Apollo Router](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/docker-router-only)

### Kubernetes

[Quickstart](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/kubernetes/quickstart)[Deploying with Extensibility](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/kubernetes/extensibility)[Enabling Metrics](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/kubernetes/metrics)[Other Considerations](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/kubernetes/other-considerations)

### AWS

[AWS ECS](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/aws)

### Azure

[Azure Container Apps](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/azure)

### GCP

[Google Cloud Run](https://www.apollographql.com/docs/graphos/routing/self-hosted/containerization/gcp)

### Managed Hosting

[Railway](https://www.apollographql.com/docs/graphos/routing/self-hosted/managed-hosting/railway)[Render](https://www.apollographql.com/docs/graphos/routing/self-hosted/managed-hosting/render)

### Apollo Cloud Routing

[Overview](https://www.apollographql.com/docs/graphos/routing/cloud)[Configuration](https://www.apollographql.com/docs/graphos/routing/cloud/configuration)[Secure Subgraphs](https://www.apollographql.com/docs/graphos/routing/cloud/secure-subgraphs)[Subscriptions](https://www.apollographql.com/docs/graphos/routing/cloud/subscriptions)[Serverless](https://www.apollographql.com/docs/graphos/routing/cloud/serverless)

### Dedicated

[Overview](https://www.apollographql.com/docs/graphos/routing/cloud/dedicated)[Quickstart](https://www.apollographql.com/docs/graphos/routing/cloud/dedicated-quickstart)[Custom Domains](https://www.apollographql.com/docs/graphos/routing/cloud/custom-domains)[Throughput Guide](https://www.apollographql.com/docs/graphos/routing/cloud/throughput-guide)[Migrate to Dedicated](https://www.apollographql.com/docs/graphos/routing/cloud/migrate-to-dedicated)

### AWS Lattice

[Lattice Configuration](https://www.apollographql.com/docs/graphos/routing/cloud/lattice-configuration)[Lattice Management](https://www.apollographql.com/docs/graphos/routing/cloud/lattice-management)[Lattice Troubleshooting](https://www.apollographql.com/docs/graphos/routing/cloud/lattice-troubleshooting)

### Tools

[Router Resource Estimator](https://www.apollographql.com/docs/graphos/routing/self-hosted/resource-estimator)[Health Checks](https://www.apollographql.com/docs/graphos/routing/self-hosted/health-checks)[Resource Management](https://www.apollographql.com/docs/graphos/routing/self-hosted/resource-management)

### Releases

[Changelogs](https://www.apollographql.com/docs/graphos/routing/changelog)[What's New in Router v2](https://www.apollographql.com/docs/graphos/routing/about-v2)[Upgrade from Router v1](https://www.apollographql.com/docs/graphos/routing/upgrade/from-router-v1)[Migrate from Gateway](https://www.apollographql.com/docs/graphos/routing/migration/from-gateway)[API Gateway Comparison](https://www.apollographql.com/docs/graphos/routing/router-api-gateway-comparison)

### GraphOS Integration

[GraphOS Plan License](https://www.apollographql.com/docs/graphos/routing/license)[GraphOS Plan Features](https://www.apollographql.com/docs/graphos/routing/graphos-features)[Apollo Uplink](https://www.apollographql.com/docs/graphos/routing/uplink)

### Reference

[Federation Version Support](https://www.apollographql.com/docs/graphos/routing/federation-version-support)[Errors](https://www.apollographql.com/docs/graphos/routing/errors)

## Resources

[Overview](https://www.apollographql.com/docs/graphos/resources)[Changelogs](https://www.apollographql.com/docs/graphos/resources/changelog)[Glossary](https://www.apollographql.com/docs/graphos/resources/glossary)[Examples](https://www.apollographql.com/docs/graphos/resources/solutions)

### Release Policies

[Apollo Feature Launch Stages](https://www.apollographql.com/docs/graphos/resources/feature-launch-stages)[GraphOS Router Release Lifecycle](https://www.apollographql.com/docs/graphos/resources/router-release-lifecycle)[Apollo Client Library Release Lifecycle](https://www.apollographql.com/docs/graphos/resources/client-library-release-lifecycle)

### Architecture

[Reference Architecture](https://www.apollographql.com/docs/graphos/resources/architecture)

### Supergraph Architecture Framework

[Overview](https://www.apollographql.com/docs/graphos/resources/saf)[Operational Excellence](https://www.apollographql.com/docs/graphos/resources/saf/operational-excellence)[Security](https://www.apollographql.com/docs/graphos/resources/saf/security)[Reliability](https://www.apollographql.com/docs/graphos/resources/saf/reliability)[Performance](https://www.apollographql.com/docs/graphos/resources/saf/performance)[Developer Experience](https://www.apollographql.com/docs/graphos/resources/saf/developer-experience)

### GraphQL Adoption Guides

[GraphQL Adoption Patterns](https://www.apollographql.com/docs/graphos/resources/guides/graphql-adoption-patterns)[Using GraphQL for Abstraction](https://www.apollographql.com/docs/graphos/resources/guides/using-graphql-for-abstraction)[Supergraph Stewardship](https://www.apollographql.com/docs/graphos/resources/guides/supergraph-stewardship)

## Rover CLI

[Overview](https://www.apollographql.com/docs/rover)[Install](https://www.apollographql.com/docs/rover/getting-started)[Configure](https://www.apollographql.com/docs/rover/configuring)[Proxy Configuration](https://www.apollographql.com/docs/rover/proxy)[CI/CD](https://www.apollographql.com/docs/rover/ci-cd)[Conventions](https://www.apollographql.com/docs/rover/conventions)[Privacy and Data Collection](https://www.apollographql.com/docs/rover/privacy)[Contributing](https://www.apollographql.com/docs/rover/contributing)[Error Codes](https://www.apollographql.com/docs/rover/errors)

### Commands

[api-key](https://www.apollographql.com/docs/rover/commands/api-key)[cloud](https://www.apollographql.com/docs/rover/commands/cloud)[config](https://www.apollographql.com/docs/rover/commands/config)[connector](https://www.apollographql.com/docs/rover/commands/connectors)[contract](https://www.apollographql.com/docs/rover/commands/contracts)[dev](https://www.apollographql.com/docs/rover/commands/dev)[explain](https://www.apollographql.com/docs/rover/commands/explain)[graph](https://www.apollographql.com/docs/rover/commands/graphs)[init](https://www.apollographql.com/docs/rover/commands/init)[license](https://www.apollographql.com/docs/rover/commands/license)[persisted-queries](https://www.apollographql.com/docs/rover/commands/persisted-queries)[readme](https://www.apollographql.com/docs/rover/commands/readmes)[subgraph](https://www.apollographql.com/docs/rover/commands/subgraphs)[supergraph](https://www.apollographql.com/docs/rover/commands/supergraphs)[template](https://www.apollographql.com/docs/rover/commands/template)

### The Apollo CLI (deprecated)

[Installation](https://www.apollographql.com/docs/rover/apollo-cli)[Validating Client Operations](https://www.apollographql.com/docs/rover/validating-client-operations)[Moving to Rover](https://www.apollographql.com/docs/rover/migration)

## IDE Support

### IDE Support

[Overview](https://www.apollographql.com/docs/ide-support)[Visual Studio Code](https://www.apollographql.com/docs/ide-support/vs-code)[JetBrains IDEs](https://www.apollographql.com/docs/ide-support/jetbrains)[Vim/NeoVim](https://www.apollographql.com/docs/ide-support/vim)

## Apollo Client (Web)

Apollo Client (Web) - v4 (latest)

[Introduction](https://www.apollographql.com/docs/react)[Why Apollo Client?](https://www.apollographql.com/docs/react/why-apollo)[Get started](https://www.apollographql.com/docs/react/get-started)

### Core concepts

[Queries](https://www.apollographql.com/docs/react/data/queries)[Suspense](https://www.apollographql.com/docs/react/data/suspense)[Fragments](https://www.apollographql.com/docs/react/data/fragments)[Mutations](https://www.apollographql.com/docs/react/data/mutations)[TypeScript](https://www.apollographql.com/docs/react/data/typescript)[Refetching](https://www.apollographql.com/docs/react/data/refetching)[Subscriptions](https://www.apollographql.com/docs/react/data/subscriptions)[Directives](https://www.apollographql.com/docs/react/data/directives)[Error handling](https://www.apollographql.com/docs/react/data/error-handling)[Persisted queries](https://www.apollographql.com/docs/react/data/persisted-queries)[Document transforms](https://www.apollographql.com/docs/react/data/document-transforms)[Deferring response data](https://www.apollographql.com/docs/react/data/defer)[Best practices](https://www.apollographql.com/docs/react/data/operation-best-practices)

### Caching

[Overview](https://www.apollographql.com/docs/react/caching/overview)[Configuration](https://www.apollographql.com/docs/react/caching/cache-configuration)[Reading and writing](https://www.apollographql.com/docs/react/caching/cache-interaction)[Garbage collection and eviction](https://www.apollographql.com/docs/react/caching/garbage-collection)[Customizing field behavior](https://www.apollographql.com/docs/react/caching/cache-field-behavior)[Memory Management](https://www.apollographql.com/docs/react/caching/memory-management)[Advanced topics](https://www.apollographql.com/docs/react/caching/advanced-topics)

### Pagination

[Overview](https://www.apollographql.com/docs/react/pagination/overview)[Core API](https://www.apollographql.com/docs/react/pagination/core-api)[Offset-based](https://www.apollographql.com/docs/react/pagination/offset-based)[Cursor-based](https://www.apollographql.com/docs/react/pagination/cursor-based)[keyArgs](https://www.apollographql.com/docs/react/pagination/key-args)

### Local State

[Overview](https://www.apollographql.com/docs/react/local-state/local-state-management)[Local-only fields](https://www.apollographql.com/docs/react/local-state/managing-state-with-field-policies)[Reactive variables](https://www.apollographql.com/docs/react/local-state/reactive-variables)[Local resolvers](https://www.apollographql.com/docs/react/local-state/local-resolvers)

### Development & Testing

[Developer tools](https://www.apollographql.com/docs/react/development-testing/developer-tooling)[GraphQL Codegen](https://www.apollographql.com/docs/react/development-testing/graphql-codegen)[Testing React components](https://www.apollographql.com/docs/react/development-testing/testing)[Schema-driven testing](https://www.apollographql.com/docs/react/development-testing/schema-driven-testing)[Mocking schema capabilities](https://www.apollographql.com/docs/react/development-testing/client-schema-mocking)[Reducing bundle size](https://www.apollographql.com/docs/react/development-testing/reducing-bundle-size)

### Performance

[Improving performance](https://www.apollographql.com/docs/react/performance/performance)[Optimistic mutation results](https://www.apollographql.com/docs/react/performance/optimistic-ui)[Server-side rendering](https://www.apollographql.com/docs/react/performance/server-side-rendering)[Compiling queries with Babel](https://www.apollographql.com/docs/react/performance/babel)

### Integrations

[Using Apollo Client with your view layer](https://www.apollographql.com/docs/react/integrations/integrations)[Integrating with React Native](https://www.apollographql.com/docs/react/integrations/react-native)[Loading queries with Webpack](https://www.apollographql.com/docs/react/integrations/webpack)

### Networking

[Basic HTTP networking](https://www.apollographql.com/docs/react/networking/basic-http-networking)[Advanced HTTP networking](https://www.apollographql.com/docs/react/networking/advanced-http-networking)[Authentication](https://www.apollographql.com/docs/react/networking/authentication)

### API Reference

### Core

[ApolloClient](https://www.apollographql.com/docs/react/api/core/ApolloClient)[InMemoryCache](https://www.apollographql.com/docs/react/api/cache/InMemoryCache)[ObservableQuery](https://www.apollographql.com/docs/react/api/core/ObservableQuery)

### Errors

[CombinedGraphQLErrors](https://www.apollographql.com/docs/react/api/errors/CombinedGraphQLErrors)[CombinedProtocolErrors](https://www.apollographql.com/docs/react/api/errors/CombinedProtocolErrors)[LinkError](https://www.apollographql.com/docs/react/api/errors/LinkError)[LocalStateError](https://www.apollographql.com/docs/react/api/errors/LocalStateError)[ServerError](https://www.apollographql.com/docs/react/api/errors/ServerError)[ServerParseError](https://www.apollographql.com/docs/react/api/errors/ServerParseError)[UnconventionalError](https://www.apollographql.com/docs/react/api/errors/UnconventionalError)

### React

[ApolloProvider](https://www.apollographql.com/docs/react/api/react/ApolloProvider)[useQuery](https://www.apollographql.com/docs/react/api/react/useQuery)[useLazyQuery](https://www.apollographql.com/docs/react/api/react/useLazyQuery)[useMutation](https://www.apollographql.com/docs/react/api/react/useMutation)[useSubscription](https://www.apollographql.com/docs/react/api/react/useSubscription)[useFragment](https://www.apollographql.com/docs/react/api/react/useFragment)[useApolloClient](https://www.apollographql.com/docs/react/api/react/useApolloClient)[useReactiveVar](https://www.apollographql.com/docs/react/api/react/useReactiveVar)[useSuspenseQuery](https://www.apollographql.com/docs/react/api/react/useSuspenseQuery)[useBackgroundQuery](https://www.apollographql.com/docs/react/api/react/useBackgroundQuery)[useReadQuery](https://www.apollographql.com/docs/react/api/react/useReadQuery)[useLoadableQuery](https://www.apollographql.com/docs/react/api/react/useLoadableQuery)[useQueryRefHandlers](https://www.apollographql.com/docs/react/api/react/useQueryRefHandlers)[skipToken](https://www.apollographql.com/docs/react/api/react/skipToken)[createQueryPreloader](https://www.apollographql.com/docs/react/api/react/preloading)[MockProvider](https://www.apollographql.com/docs/react/api/react/testing)[SSR](https://www.apollographql.com/docs/react/api/react/ssr)

### Apollo Link

[Overview](https://www.apollographql.com/docs/react/api/link/introduction)[ApolloLink](https://www.apollographql.com/docs/react/api/link/apollo-link)[BaseHttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-base-http)[BaseBatchHttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-base-batch-http)[BatchHttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-batch-http)[BatchLink](https://www.apollographql.com/docs/react/api/link/apollo-link-batch)[ClientAwarenessLink](https://www.apollographql.com/docs/react/api/link/apollo-link-client-awareness)[ErrorLink](https://www.apollographql.com/docs/react/api/link/apollo-link-error)[GraphQLWsLink](https://www.apollographql.com/docs/react/api/link/apollo-link-subscriptions)[HttpLink](https://www.apollographql.com/docs/react/api/link/apollo-link-http)[PersistedQueryLink](https://www.apollographql.com/docs/react/api/link/apollo-link-persisted-queries)[RemoveTypenameFromVariablesLink](https://www.apollographql.com/docs/react/api/link/apollo-link-remove-typename)[RetryLink](https://www.apollographql.com/docs/react/api/link/apollo-link-retry)[SchemaLink](https://www.apollographql.com/docs/react/api/link/apollo-link-schema)[SetContextLink](https://www.apollographql.com/docs/react/api/link/apollo-link-context)[WebSocketLink (deprecated)](https://www.apollographql.com/docs/react/api/link/apollo-link-ws)[Community links](https://www.apollographql.com/docs/react/api/link/community-links)

[Changelog](https://github.com/apollographql/apollo-client/blob/main/CHANGELOG.md)[Migrating to Apollo Client 4.0](https://www.apollographql.com/docs/react/migrating/apollo-client-4-migration)[Versioning Policy](https://www.apollographql.com/docs/react/versioning-policy)

## Apollo iOS

Apollo iOS - v2 (latest)

[Introduction](https://www.apollographql.com/docs/ios)[Get Started](https://www.apollographql.com/docs/ios/get-started)

### Project Configuration

[Introduction](https://www.apollographql.com/docs/ios/project-configuration/intro)[1\. Project Modularization](https://www.apollographql.com/docs/ios/project-configuration/modularization)[2\. Schema Types](https://www.apollographql.com/docs/ios/project-configuration/schema-types)[3\. Operation Models](https://www.apollographql.com/docs/ios/project-configuration/operation-models)[SDK components](https://www.apollographql.com/docs/ios/project-configuration/sdk-components)

### Migration Guides

[v2.0](https://www.apollographql.com/docs/ios/migrations/2.0)

### Tutorial

[Code Generation](https://www.apollographql.com/docs/ios/tutorial/codegen-getting-started)

### Build a project with Apollo

[Introduction](https://www.apollographql.com/docs/ios/tutorial/tutorial-introduction)[1\. Configure your project](https://www.apollographql.com/docs/ios/tutorial/tutorial-configure-project)[2\. Add the GraphQL schema](https://www.apollographql.com/docs/ios/tutorial/tutorial-add-graphql-schema)[3\. Write your first query](https://www.apollographql.com/docs/ios/tutorial/tutorial-write-your-first-query)[4\. Running code generation](https://www.apollographql.com/docs/ios/tutorial/tutorial-running-code-generation)[5\. Execute your first query](https://www.apollographql.com/docs/ios/tutorial/tutorial-execute-first-query)[6\. Connect your queries to your UI](https://www.apollographql.com/docs/ios/tutorial/tutorial-connect-queries-to-ui)[7\. Add more info to the list](https://www.apollographql.com/docs/ios/tutorial/tutorial-add-more-info-to-list)[8\. Paginate results](https://www.apollographql.com/docs/ios/tutorial/tutorial-paginate-results)[9\. Complete the details view](https://www.apollographql.com/docs/ios/tutorial/tutorial-complete-details-view)[10\. Write your first mutation](https://www.apollographql.com/docs/ios/tutorial/tutorial-first-mutation)[11\. Authenticate your operations](https://www.apollographql.com/docs/ios/tutorial/tutorial-authenticate-operations)[12\. Define additional mutations](https://www.apollographql.com/docs/ios/tutorial/tutorial-define-additional-mutations)

[API Reference](https://www.apollographql.com/docs/ios/docc/documentation)

### Code Generation

[Introduction](https://www.apollographql.com/docs/ios/code-generation/introduction)[The Codegen CLI](https://www.apollographql.com/docs/ios/code-generation/codegen-cli)[Configuration](https://www.apollographql.com/docs/ios/code-generation/codegen-configuration)[Downloading a Schema](https://www.apollographql.com/docs/ios/code-generation/downloading-schema)[Running Code Generation in Swift Code](https://www.apollographql.com/docs/ios/code-generation/run-codegen-in-swift-code)[Code Generation Troubleshooting](https://www.apollographql.com/docs/ios/troubleshooting/codegen-troubleshooting)

### Fetching

[Fetching Data](https://www.apollographql.com/docs/ios/fetching/fetching-data)

### Operations

[Queries](https://www.apollographql.com/docs/ios/fetching/queries)[Mutations](https://www.apollographql.com/docs/ios/fetching/mutations)[Subscriptions](https://www.apollographql.com/docs/ios/fetching/subscriptions)[Fragments](https://www.apollographql.com/docs/ios/fetching/fragments)

[Operation Arguments](https://www.apollographql.com/docs/ios/fetching/operation-arguments)[Error Handling](https://www.apollographql.com/docs/ios/fetching/error-handling)[Type Conditions](https://www.apollographql.com/docs/ios/fetching/type-conditions)[Custom Scalars](https://www.apollographql.com/docs/ios/custom-scalars)[Persisted Queries](https://www.apollographql.com/docs/ios/fetching/persisted-queries)[@defer support](https://www.apollographql.com/docs/ios/fetching/defer)

### Caching

[Introduction](https://www.apollographql.com/docs/ios/caching/introduction)[Setup](https://www.apollographql.com/docs/ios/caching/cache-setup)[Direct Cache Access](https://www.apollographql.com/docs/ios/caching/cache-transactions)[Custom Cache Keys](https://www.apollographql.com/docs/ios/caching/cache-key-resolution)[Programmatic Cache Keys](https://www.apollographql.com/docs/ios/caching/programmatic-cache-keys)

### Networking

[Creating a Client](https://www.apollographql.com/docs/ios/networking/client-creation)

### Pagination

[Introduction](https://www.apollographql.com/docs/ios/pagination/introduction)[Using Custom Response Models](https://www.apollographql.com/docs/ios/pagination/custom-types)[Directional Pagination](https://www.apollographql.com/docs/ios/pagination/directional-pagers)[Multi-query Pagination](https://www.apollographql.com/docs/ios/pagination/multi-query)

### Development & Testing

[Test Mocks](https://www.apollographql.com/docs/ios/testing/test-mocks)[Including Apollo as an XCFramework](https://www.apollographql.com/docs/ios/development/using-xcframework)

### Advanced

[File Uploads](https://www.apollographql.com/docs/ios/advanced/file-uploads)[Request Chain Customization](https://www.apollographql.com/docs/ios/advanced/request-chain)[Client Directives](https://www.apollographql.com/docs/ios/client-directives)

## Apollo Kotlin

Apollo Kotlin - v4 (latest)

[Get Started](https://www.apollographql.com/docs/kotlin)[Migrating to v4](https://www.apollographql.com/docs/kotlin/migration/4.0)[Modules](https://www.apollographql.com/docs/kotlin/essentials/modules)[Evolution policy](https://www.apollographql.com/docs/kotlin/essentials/evolution)[Tutorial](https://www.apollographql.com/tutorials/apollo-kotlin-android-part1)[Kdoc](https://apollographql.github.io/apollo-kotlin/kdoc/older/4.2.0/)[Changelog](https://github.com/apollographql/apollo-kotlin/blob/main/CHANGELOG.md)

### Configuration

[Gradle plugin configuration](https://www.apollographql.com/docs/kotlin/advanced/plugin-configuration)[Gradle plugin recipes](https://www.apollographql.com/docs/kotlin/advanced/plugin-recipes)[Multi Modules](https://www.apollographql.com/docs/kotlin/advanced/multi-modules)[File types](https://www.apollographql.com/docs/kotlin/essentials/file-types)[Client Awareness](https://www.apollographql.com/docs/kotlin/advanced/client-awareness)

### Fetching

[Queries](https://www.apollographql.com/docs/kotlin/essentials/queries)[Mutations](https://www.apollographql.com/docs/kotlin/essentials/mutations)[Subscriptions](https://www.apollographql.com/docs/kotlin/essentials/subscriptions)[GraphQL variables](https://www.apollographql.com/docs/kotlin/advanced/operation-variables)[Error handling](https://www.apollographql.com/docs/kotlin/essentials/errors)[Custom scalars](https://www.apollographql.com/docs/kotlin/essentials/custom-scalars)[Fragments](https://www.apollographql.com/docs/kotlin/essentials/fragments)[@defer support](https://www.apollographql.com/docs/kotlin/fetching/defer)[Persisted queries](https://www.apollographql.com/docs/kotlin/advanced/persisted-queries)

### Caching

[Introduction](https://www.apollographql.com/docs/kotlin/caching/introduction)[Normalized caches](https://www.apollographql.com/docs/kotlin/caching/normalized-cache)[Declarative cache IDs](https://www.apollographql.com/docs/kotlin/caching/declarative-ids)[Programmatic cache IDs](https://www.apollographql.com/docs/kotlin/caching/programmatic-ids)[Watching cached data](https://www.apollographql.com/docs/kotlin/caching/query-watchers)[ApolloStore](https://www.apollographql.com/docs/kotlin/caching/store)[HTTP cache](https://www.apollographql.com/docs/kotlin/caching/http-cache)[Troubleshooting](https://www.apollographql.com/docs/kotlin/caching/troubleshooting)

### Networking

[Interceptors](https://www.apollographql.com/docs/kotlin/advanced/interceptors-http)[Custom HTTP clients](https://www.apollographql.com/docs/kotlin/advanced/http-engine)[Using the models without apollo-runtime](https://www.apollographql.com/docs/kotlin/advanced/no-runtime)[Authentication](https://www.apollographql.com/docs/kotlin/advanced/authentication)[WebSocket errors](https://www.apollographql.com/docs/kotlin/advanced/websocket-errors)[Batching operations](https://www.apollographql.com/docs/kotlin/advanced/query-batching)

### Development & Testing

[Testing overview](https://www.apollographql.com/docs/kotlin/testing/overview)[Mocking HTTP responses](https://www.apollographql.com/docs/kotlin/testing/mocking-http-responses)[Mocking GraphQL responses](https://www.apollographql.com/docs/kotlin/testing/mocking-graphql-responses)[Data builders](https://www.apollographql.com/docs/kotlin/testing/data-builders)[Android Studio plugin](https://www.apollographql.com/docs/kotlin/testing/android-studio-plugin)[Apollo Debug Server](https://www.apollographql.com/docs/kotlin/testing/apollo-debug-server)

### Advanced

[Uploading files](https://www.apollographql.com/docs/kotlin/advanced/upload)[Monitoring the network state](https://www.apollographql.com/docs/kotlin/advanced/network-connectivity)[Handling nullability](https://www.apollographql.com/docs/kotlin/advanced/nullability)[Experimental WebSockets](https://www.apollographql.com/docs/kotlin/advanced/experimental-websockets)[Using aliases](https://www.apollographql.com/docs/kotlin/advanced/using-aliases)[Using Java](https://www.apollographql.com/docs/kotlin/advanced/java)[Apollo AST](https://www.apollographql.com/docs/kotlin/advanced/apollo-ast)[Compiler plugins](https://www.apollographql.com/docs/kotlin/advanced/compiler-plugins)[JS Interoperability](https://www.apollographql.com/docs/kotlin/advanced/js-interop)[Response based codegen](https://www.apollographql.com/docs/kotlin/advanced/response-based)[Apollo Kotlin galaxy](https://www.apollographql.com/docs/kotlin/advanced/galaxy)

## Apollo Server

Apollo Server - v4âv5 (latest)

[Introduction](https://www.apollographql.com/docs/apollo-server)[Get started](https://www.apollographql.com/docs/apollo-server/getting-started)

### New in v4 and v5

[Migrating from Apollo Server 4](https://www.apollographql.com/docs/apollo-server/migration)[Migrating from Apollo Server 3](https://www.apollographql.com/docs/apollo-server/migration-from-v3)[Previous versions](https://www.apollographql.com/docs/apollo-server/previous-versions)[Changelog](https://github.com/apollographql/apollo-server/blob/main/packages/server/CHANGELOG.md)

### Defining a Schema

[Schema basics](https://www.apollographql.com/docs/apollo-server/schema/schema)[Unions and interfaces](https://www.apollographql.com/docs/apollo-server/schema/unions-interfaces)[Custom scalars](https://www.apollographql.com/docs/apollo-server/schema/custom-scalars)[Directives](https://www.apollographql.com/docs/apollo-server/schema/directives)

### Resolving Operations

[Resolvers](https://www.apollographql.com/docs/apollo-server/data/resolvers)[Sharing context](https://www.apollographql.com/docs/apollo-server/data/context)[Error handling](https://www.apollographql.com/docs/apollo-server/data/errors)[Subscriptions](https://www.apollographql.com/docs/apollo-server/data/subscriptions)

### Fetching Data

[Overview](https://www.apollographql.com/docs/apollo-server/data/fetching-data)[REST APIs](https://www.apollographql.com/docs/apollo-server/data/fetching-rest)

### Web Frameworks

[Integrations](https://www.apollographql.com/docs/apollo-server/integrations/integration-index)[Building integrations](https://www.apollographql.com/docs/apollo-server/integrations/building-integrations)[MERN stack tutorial](https://www.apollographql.com/docs/apollo-server/integrations/mern)

### Development Workflow

[Build and run queries](https://www.apollographql.com/docs/apollo-server/workflow/build-run-queries)[Request format](https://www.apollographql.com/docs/apollo-server/workflow/requests)[Generating TS types](https://www.apollographql.com/docs/apollo-server/workflow/generate-types)[Mocking](https://www.apollographql.com/docs/apollo-server/testing/mocking)[Integration testing](https://www.apollographql.com/docs/apollo-server/testing/testing)[Apollo Studio Explorer](https://www.apollographql.com/docs/studio/explorer/)

### Performance

[Caching](https://www.apollographql.com/docs/apollo-server/performance/caching)[Cache backends](https://www.apollographql.com/docs/apollo-server/performance/cache-backends)[Response Cache Eviction](https://www.apollographql.com/docs/apollo-server/performance/response-cache-eviction)[Automatic persisted queries](https://www.apollographql.com/docs/apollo-server/performance/apq)

### Security

[Auth](https://www.apollographql.com/docs/apollo-server/security/authentication)[CORS](https://www.apollographql.com/docs/apollo-server/security/cors)[Terminating SSL](https://www.apollographql.com/docs/apollo-server/security/terminating-ssl)[Proxy configuration](https://www.apollographql.com/docs/apollo-server/security/proxy-configuration)

### Deployment

[Lambda](https://www.apollographql.com/docs/apollo-server/deployment/lambda)[Heroku](https://www.apollographql.com/docs/apollo-server/deployment/heroku)

### Monitoring

[Metrics and logging](https://www.apollographql.com/docs/apollo-server/monitoring/metrics)[Health checks](https://www.apollographql.com/docs/apollo-server/monitoring/health-checks)

### API Reference

[ApolloServer](https://www.apollographql.com/docs/apollo-server/api/apollo-server)[startStandaloneServer](https://www.apollographql.com/docs/apollo-server/api/standalone)[expressMiddleware](https://www.apollographql.com/docs/apollo-server/api/express-middleware)

### Plugins

[Overview](https://www.apollographql.com/docs/apollo-server/builtin-plugins)

### Built-in

[Usage reporting](https://www.apollographql.com/docs/apollo-server/api/plugin/usage-reporting)[Schema reporting](https://www.apollographql.com/docs/apollo-server/api/plugin/schema-reporting)[Inline trace](https://www.apollographql.com/docs/apollo-server/api/plugin/inline-trace)[Drain HTTP server](https://www.apollographql.com/docs/apollo-server/api/plugin/drain-http-server)[Cache control](https://www.apollographql.com/docs/apollo-server/api/plugin/cache-control)[Landing pages](https://www.apollographql.com/docs/apollo-server/api/plugin/landing-pages)[Federated subscriptions](https://www.apollographql.com/docs/apollo-server/api/plugin/subscription-callback)

### Custom

[Creating plugins](https://www.apollographql.com/docs/apollo-server/integrations/plugins)[Event reference](https://www.apollographql.com/docs/apollo-server/integrations/plugins-event-reference)

### Using with Federation

### As a subgraph

[Setup](https://www.apollographql.com/docs/apollo-server/using-federation/apollo-subgraph-setup)[@apollo/subgraph reference](https://www.apollographql.com/docs/apollo-server/using-federation/api/apollo-subgraph)

### As a gateway

[Setup](https://www.apollographql.com/docs/apollo-server/using-federation/apollo-gateway-setup)[Gateway performance](https://www.apollographql.com/docs/apollo-server/using-federation/gateway-performance)[@apollo/gateway reference](https://www.apollographql.com/docs/apollo-server/using-federation/api/apollo-gateway)

## Apollo MCP Server

Apollo MCP Server - v1 (latest)

[Overview](https://www.apollographql.com/docs/apollo-mcp-server)[Quickstart](https://www.apollographql.com/docs/apollo-mcp-server/quickstart)[Define tools](https://www.apollographql.com/docs/apollo-mcp-server/define-tools)

### Configuration

[YAML Config Reference](https://www.apollographql.com/docs/apollo-mcp-server/config-file)[Custom Scalars](https://www.apollographql.com/docs/apollo-mcp-server/custom-scalars)

[Run the MCP Server](https://www.apollographql.com/docs/apollo-mcp-server/run)[Debugging](https://www.apollographql.com/docs/apollo-mcp-server/debugging)

### Deployment

[Overview](https://www.apollographql.com/docs/apollo-mcp-server/deploy)[Health Checks](https://www.apollographql.com/docs/apollo-mcp-server/health-checks)[CORS](https://www.apollographql.com/docs/apollo-mcp-server/cors)

[Authorization](https://www.apollographql.com/docs/apollo-mcp-server/auth)[Telemetry](https://www.apollographql.com/docs/apollo-mcp-server/telemetry)[Best Practices](https://www.apollographql.com/docs/apollo-mcp-server/best-practices)[Licensing](https://www.apollographql.com/docs/apollo-mcp-server/licensing)[Limitations](https://www.apollographql.com/docs/apollo-mcp-server/limitations)

### Guides

[Authorization with Auth0](https://www.apollographql.com/docs/apollo-mcp-server/guides/auth-auth0)

## Apollo Operator

[Overview](https://www.apollographql.com/docs/apollo-operator)

### Getting Started

[Install Operator](https://www.apollographql.com/docs/apollo-operator/get-started/install-operator)[Add Subgraphs](https://www.apollographql.com/docs/apollo-operator/get-started/add-subgraphs)[Deploy Supergraphs](https://www.apollographql.com/docs/apollo-operator/get-started/deploy-supergraph)

### Kubernetes Resources

### Supergraph

[Overview](https://www.apollographql.com/docs/apollo-operator/resources/supergraph)[Autoscaling your Supergraphs](https://www.apollographql.com/docs/apollo-operator/resources/supergraph/autoscalers)[Supergraph podTemplate](https://www.apollographql.com/docs/apollo-operator/resources/supergraph/pod_template)[Supergraph networking](https://www.apollographql.com/docs/apollo-operator/resources/supergraph/networking)

[SupergraphSchema](https://www.apollographql.com/docs/apollo-operator/resources/supergraphschema)[SupergraphSet](https://www.apollographql.com/docs/apollo-operator/resources/supergraphset)[Subgraph](https://www.apollographql.com/docs/apollo-operator/resources/subgraph)

### Workflows

[Overview](https://www.apollographql.com/docs/apollo-operator/workflows)[Single Cluster Setup](https://www.apollographql.com/docs/apollo-operator/workflows/single-cluster)[Multi-Cluster & Hybrid Setup](https://www.apollographql.com/docs/apollo-operator/workflows/multi-cluster)[Deploy Only](https://www.apollographql.com/docs/apollo-operator/workflows/deploy-only)

### Configuration

[Overview](https://www.apollographql.com/docs/apollo-operator/configuration)[Telemetry](https://www.apollographql.com/docs/apollo-operator/configuration/telemetry)[Controllers](https://www.apollographql.com/docs/apollo-operator/configuration/controllers)[OCI](https://www.apollographql.com/docs/apollo-operator/configuration/oci)[Private Registries](https://www.apollographql.com/docs/apollo-operator/configuration/private-registries)

[Security Best Practices](https://www.apollographql.com/docs/apollo-operator/security-best-practices)

[Apollo Client (Web)](react)

## 

On this page

Copy as MD

  * Prerequisites 

  * Executing a mutation 

  * Example 

  * Providing options 

  * Tracking mutation status 

  * Resetting mutation status 

  * Updating local data 

  * Supported methods 

  * Refetching queries 

  * Updating the cache directly 

  * Include modified objects in mutation responses 

  * The update function 

  * Refetching after update 

  * useMutation API 

  * Options 

  * Result 

  * Next steps 




[ Home ](/docs/)

/ [ Apollo Client (Web) ](/docs/react)

/ [ Core concepts ](/docs/react/data/queries)

/ [ Mutations ](/docs/react/data/mutations)

[ Apollo Client (Web) / ](/docs/react)

[ Core concepts ](/docs/react/data/queries)

#  Mutations in Apollo Client 

Modify data with the useMutation hook

Ask AI a question about this page

Ask with ChatGPT

Ask with ClaudeAsk with ChatGPT

* * *

Now that we've [learned how to query data](https://www.apollographql.com/docs/react/data/queries/) from our backend with Apollo Client, the natural next step is to learn how to _modify_ back-end data with **mutations**.

This article demonstrates how to send updates to your GraphQL server with the `useMutation` hook. You'll also learn how to update the Apollo Client cache after executing a mutation, and how to track loading and error states.

> To follow along with the examples below, open up our [starter project](https://codesandbox.io/s/mutations-example-app-start-gm7i5) and [sample GraphQL server](https://codesandbox.io/s/mutations-example-app-server-sxewr) on CodeSandbox. You can view the completed version of the app [here](https://codesandbox.io/s/mutations-example-app-final-tjoje).

## Prerequisites

**note**

If your application is built using TypeScript, we recommend reading the [TypeScript guide](https://www.apollographql.com/docs/react/development-testing/static-typing) to learn how to use TypeScript with Apollo Client.

This article assumes you're familiar with building basic GraphQL mutations. If you need a refresher, we recommend that you [read this guide](http://graphql.org/learn/queries/#mutations).

This article also assumes that you've already set up Apollo Client and have wrapped your React app in an `ApolloProvider` component. For help with those steps, [get started](https://www.apollographql.com/docs/react/get-started/).

## Executing a mutation

The `useMutation` [React hook](https://react.dev/reference/react) is the primary API for executing mutations in an Apollo application.

To execute a mutation, you first call `useMutation` within a React component and pass it the mutation you want to execute, like so:

JavaScript

my-component.jsx

copy
    
    
    1import { gql } from "@apollo/client";
    2import { useMutation } from "@apollo/client/react";
    3
    4// Define mutation
    5const INCREMENT_COUNTER = gql`
    6  # Increments a back-end counter and gets its resulting value
    7  mutation IncrementCounter {
    8    incrementCounter {
    9      currentValue
    10    }
    11  }
    12`;
    13
    14function MyComponent() {
    15  const [mutate, { data, loading, error }] = useMutation(INCREMENT_COUNTER);
    16}

As shown above, you use the `gql` function to parse the mutation string into a GraphQL document that you then pass to `useMutation`.

When your component renders, `useMutation` returns a tuple that includes:

  * A **mutate function** that you can call at any time to execute the mutation

    * Unlike `useQuery`, `useMutation` _doesn't_ execute its operation automatically on render. Instead, call the `mutate` function to execute the mutation.

  * An object with fields that represent the current status of the mutation's execution (`data`, `loading`, etc.)

    * This object is similar to the object returned by the `useQuery` hook. For details, see [Result](https://www.apollographql.com/docs/react/data/mutations#result).




### Example

Let's say we're creating a to-do list application and we want the user to be able to add items to their list. First, we'll create a corresponding GraphQL mutation named `ADD_TODO`. Remember to wrap GraphQL strings in the `gql` function to parse them into query documents:

JavaScript

add-todo.jsx

copy
    
    
    1import { gql } from "@apollo/client";
    2import { useMutation } from "@apollo/client/react";
    3
    4const ADD_TODO = gql`
    5  mutation AddTodo($type: String!) {
    6    addTodo(type: $type) {
    7      id
    8      type
    9    }
    10  }
    11`;

Next, we'll create a component named `AddTodo` that represents the submission form for the to-do list. Inside it, we'll pass our `ADD_TODO` mutation to the `useMutation` hook:

JavaScript

add-todo.jsx

copy
    
    
    1function AddTodo() {
    2  const [value, setValue] = useState("");
    3  const [addTodo, { data, loading, error }] = useMutation(ADD_TODO);
    4
    5  if (loading) return "Submitting...";
    6  if (error) return `Submission error! ${error.message}`;
    7
    8  return (
    9    <div>
    10      <form
    11        onSubmit={(e) => {
    12          e.preventDefault();
    13          addTodo({ variables: { type: value } });
    14          setValue("");
    15        }}
    16      >
    17        <input
    18          value={value}
    19          onChange={(e) => {
    20            setValue(e.target.value);
    21          }}
    22        />
    23        <button type="submit">Add Todo</button>
    24      </form>
    25    </div>
    26  );
    27}

In this example, our form's `onSubmit` handler calls the **mutate function** (named `addTodo`) that's returned by the `useMutation` hook. This tells Apollo Client to execute the mutation by sending it to our GraphQL server.

**note**

`useMutation` behaves differently than [`useQuery`](https://www.apollographql.com/docs/react/data/queries/), which executes its operation as soon as its component renders. This is because mutations are more commonly executed in response to a user action (such as submitting a form in this case).

### Providing options

The `useMutation` hook accepts an `options` object as its second parameter. Here's an example that provides some default values for GraphQL `variables`:

JavaScript

copy
    
    
    1const [addTodo, { data, loading, error }] = useMutation(ADD_TODO, {
    2  variables: {
    3    type: "placeholder",
    4    someOtherVariable: 1234,
    5  },
    6});

You can _also_ provide options directly to your mutate function, as demonstrated in this snippet from [the example above](https://www.apollographql.com/docs/react/data/mutations#example):

JavaScript

copy
    
    
    1addTodo({
    2  variables: {
    3    type: value,
    4  },
    5});

Here, we use the `variables` option to provide the values of any GraphQL variables that our mutation requires (specifically, the `type` of the created to-do item).

Learn more about the available options in [Options](https://www.apollographql.com/docs/react/data/mutations#options).

#### Option precedence

If you provide the same option to both `useMutation` _and_ your mutate function, the mutate function's value takes precedence. In the specific case of the `variables` option, the two objects are merged _shallowly_ , which means any variables provided only to `useMutation` are preserved in the resulting object. This helps you set default values for variables.

In [the example snippets above](https://www.apollographql.com/docs/react/data/mutations#providing-options), `value` would override `"placeholder"` as the value of the `type` variable. The value of `someOtherVariable` (`1234`) would be preserved.

**note**

When using TypeScript, you might see an error related to a missing variable when a required variable is not provided to either the hook or the `mutate` function. Providing required variables to the hook makes them optional in the `mutate` function. If a required variable is not provided to the hook, it is required in the `mutate` function.

### Tracking mutation status

In addition to a mutate function, the `useMutation` hook returns an object that represents the current state of the mutation's execution. The fields of this object include booleans that indicate whether the mutate function has been `called` and whether the mutation's result is currently `loading`.

[The example above](https://www.apollographql.com/docs/react/data/mutations#example) destructures the `loading` and `error` fields from this object to render the `AddTodo` component differently depending on the mutation's current status:

JavaScript

copy
    
    
    1if (loading) return "Submitting...";
    2if (error) return `Submission error! ${error.message}`;

**note**

The `useMutation` hook supports `onCompleted` and `onError` options if you need to perform side effects when the mutation completes. See the [API reference](https://www.apollographql.com/docs/react/api/react/useMutation) for more details.

Learn more about result object in [Result](https://www.apollographql.com/docs/react/data/mutations#result).

### Resetting mutation status

The mutation result object returned by `useMutation` includes a [`reset` function](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-reset):

JavaScript

copy
    
    
    1const [login, { reset }] = useMutation(LOGIN_MUTATION);

Call `reset` to reset the mutation's result to its initial state (i.e., _before_ the mutate function was called). You can use this to enable users to dismiss mutation result data or errors in the UI.

**note**

Calling `reset` does _not_ remove any cached data returned by the mutation's execution. It only affects the state associated with the `useMutation` hook, causing the corresponding component to rerender.

JavaScript

copy
    
    
    1function LoginPage() {
    2  const [login, { error, reset }] = useMutation(LOGIN_MUTATION);
    3
    4  return (
    5    <>
    6      <form>
    7        <input class="login" />
    8        <input class="password" />
    9        <button onclick={login}>Login</button>
    10      </form>
    11      {error && (
    12        <LoginFailedMessageWindow
    13          message={error.message}
    14          onDismiss={() => reset()}
    15        />
    16      )}
    17    </>
    18  );
    19}

## Updating local data

When you execute a mutation, you modify back-end data. Usually, you then want to update your _locally cached_ data to reflect the back-end modification. For example, if you execute a mutation to add an item to your to-do list, you also want that item to appear in your cached copy of the list.

### Supported methods

The most straightforward way to update your local data is to [refetch any queries](https://www.apollographql.com/docs/react/data/mutations#refetching-queries) that might be affected by the mutation. However, this method requires additional network requests.

If your mutation returns all of the objects and fields that it modified, you can [update your cache directly](https://www.apollographql.com/docs/react/data/mutations#updating-the-cache-directly) _without_ making any followup network requests.

We recommend reading the guide on [Caching in Apollo Client](https://www.apollographql.com/docs/react/caching/overview) to understand how data is stored in the cache and how to perform updates to that data.

## Refetching queries

You can refetch queries after a mutation by providing a `refetchQueries` option to `useMutation`:

JavaScript

copy
    
    
    1// Refetches two queries after mutation completes
    2const [addTodo, { data, loading, error }] = useMutation(ADD_TODO, {
    3  refetchQueries: [
    4    GET_POST, // DocumentNode object parsed with gql
    5    "GetComments", // Query name
    6  ],
    7});

You can provide one of the following values to `refetchQueries`:

  * A `refetchQueries` array to refetch specific queries

  * The shorthand `"active"` string to refetch all active queries

  * The shorthand `"all"` string to refetch all active and inactive queries




It is most common to provide a `refetchQueries` array when performing mutations. Learn more about active and inactive queries in the [Refetching](https://www.apollographql.com/docs/react/data/refetching) guide.

When providing `refetchQueries` as an array, each element in the `refetchQueries` array is one of the following:

  * A `DocumentNode` object parsed with the `gql` function

  * The name of a query as a string (e.g., `GetComments`)

    * To refer to queries by name, make sure each of your app's queries have a _unique_ name.




**note**

Each query in the `refetchQueries` array must be an _active_ query. If an inactive or unknown query is provided, a warning will be logged to the console.

Each included query is executed with its most recently provided set of variables.

You can provide the `refetchQueries` option either to `useMutation` or to the mutate function. For details, see [Option precedence](https://www.apollographql.com/docs/react/data/mutations#option-precedence).

## Updating the cache directly

### Include modified objects in mutation responses

As a best practice, a mutation response should include any object(s) the mutation modified. This enables Apollo Client to normalize those objects and cache them according to their `__typename` and [`keyFields`](https://www.apollographql.com/docs/react/caching/cache-configuration/#customizing-cache-ids).

[In the example above](https://www.apollographql.com/docs/react/data/mutations#example), our `ADD_TODO` mutation might return a `Todo` object with the following structure:

JSON

copy
    
    
    1{
    2  "__typename": "Todo",
    3  "id": "5",
    4  "type": "groceries"
    5}

**note**

Apollo Client automatically adds the `__typename` field to every object in your queries and mutations by default.

Upon receiving this response object, Apollo Client caches it with key `Todo:5`. If a cached object _already_ exists with this key, Apollo Client overwrites any existing fields that are also included in the mutation response (other existing fields are preserved).

Returning modified objects like this is a helpful first step to keeping your cache in sync with your backend. However, it isn't always sufficient. For example, a newly cached object isn't automatically added to any _list fields_ that should now include that object. To accomplish this, you can define an [`update` function](https://www.apollographql.com/docs/react/data/mutations#the-update-function).

### The `update` function

When a [mutation's response](https://www.apollographql.com/docs/react/data/mutations#include-modified-objects-in-mutation-responses) is insufficient to update _all_ modified fields in your cache (such as certain list fields), you can define an `update` function to apply manual changes to your cached data after a mutation.

You provide an `update` function to `useMutation`, like so:

JavaScript

copy
    
    
    1const GET_TODOS = gql`
    2  query GetTodos {
    3    todos {
    4      id
    5    }
    6  }
    7`;
    8
    9function AddTodo() {
    10  let input;
    11  const [addTodo] = useMutation(ADD_TODO, {
    12    update(cache, { data: { addTodo } }) {
    13      cache.modify({
    14        fields: {
    15          todos(existingTodos = []) {
    16            const newTodoRef = cache.writeFragment({
    17              data: addTodo,
    18              fragment: gql`
    19                fragment NewTodo on Todo {
    20                  id
    21                  type
    22                }
    23              `,
    24            });
    25            return [...existingTodos, newTodoRef];
    26          },
    27        },
    28      });
    29    },
    30  });
    31
    32  return (
    33    <div>
    34      <form
    35        onSubmit={(e) => {
    36          e.preventDefault();
    37          addTodo({ variables: { type: input.value } });
    38          input.value = "";
    39        }}
    40      >
    41        <input
    42          ref={(node) => {
    43            input = node;
    44          }}
    45        />
    46        <button type="submit">Add Todo</button>
    47      </form>
    48    </div>
    49  );
    50}

As shown, the `update` function is passed a `cache` object that represents the Apollo Client cache. This object provides access to cache API methods like `readQuery`/`writeQuery`, `readFragment`/`writeFragment`, `modify`, and `evict`. These methods enable you to execute GraphQL operations on the cache as though you're interacting with a GraphQL server.

> Learn more about supported cache functions in [Interacting with cached data](https://www.apollographql.com/docs/react/caching/cache-interaction/).

The `update` function is _also_ passed an object with a `data` property that contains the result of the mutation. You can use this value to update the cache with `cache.writeQuery`, `cache.writeFragment`, or `cache.modify`.

**note**

If your mutation specifies an [optimistic response](https://www.apollographql.com/docs/react/performance/optimistic-ui/), your `update` function is called **twice** : once with the optimistic result, and again with the actual result of the mutation when it returns.

When the `ADD_TODO` mutation executes in the above example, the newly added and returned `addTodo` object is automatically saved into the cache _before_ the `update` function runs. However, the cached list of `ROOT_QUERY.todos` (which is watched by the `GET_TODOS` query) is _not_ automatically updated. This means that the `GET_TODOS` query isn't notified of the new `Todo` object, which in turn means that the query doesn't update to show the new item.

To address this, we use `cache.modify` to surgically insert or delete items from the cache, by running "modifier" functions. In the example above, we know the results of the `GET_TODOS` query are stored in the `ROOT_QUERY.todos` array in the cache, so we use a `todos` modifier function to update the cached array to include a reference to the newly added `Todo`. With the help of `cache.writeFragment`, we get an internal reference to the added `Todo`, then append that reference to the `ROOT_QUERY.todos` array.

Any changes you make to cached data inside of an `update` function are automatically broadcast to queries that are listening for changes to that data. Consequently, your application's UI will update to reflect these updated cached values.

### Refetching after `update`

An `update` function attempts to replicate a mutation's back-end modifications in your client's local cache. These cache modifications are broadcast to all affected active queries, which updates your UI automatically. If the `update` function does this correctly, your users see the latest data immediately, without needing to await another network round trip.

However, an `update` function might get this replication _wrong_ by setting a cached value incorrectly. You can "double check" your `update` function's modifications by refetching affected active queries. To do so, you first provide an `onQueryUpdated` callback function to your mutate function:

JavaScript

copy
    
    
    1addTodo({
    2  variables: { type: input.value },
    3  update(cache, result) {
    4    // Update the cache as an approximation of server-side mutation effects
    5  },
    6  onQueryUpdated(observableQuery) {
    7    // Define any custom logic for determining whether to refetch
    8    if (shouldRefetchQuery(observableQuery)) {
    9      return observableQuery.refetch();
    10    }
    11  },
    12});

After your `update` function completes, Apollo Client calls `onQueryUpdated` _once for each active query with cached fields that were updated_. Within `onQueryUpdated`, you can use any custom logic to determine whether you want to refetch the associated query.

To refetch a query from `onQueryUpdated`, call `return observableQuery.refetch()`, as shown above. Otherwise, no return value is required. If a refetched query's response differs from your `update` function's modifications, your cache and UI are both automatically updated again. Otherwise, your users see no change.

Occasionally, it might be difficult to make your `update` function update all relevant queries. Not every mutation returns enough information for the `update` function to do its job effectively. To make absolutely sure a certain query is included, you can combine `onQueryUpdated` with `refetchQueries: [...]`:

JavaScript

copy
    
    
    1addTodo({
    2  variables: { type: input.value },
    3  update(cache, result) {
    4    // Update the cache as an approximation of server-side mutation effects.
    5  },
    6  // Force ReallyImportantQuery to be passed to onQueryUpdated.
    7  refetchQueries: ["ReallyImportantQuery"],
    8  onQueryUpdated(observableQuery) {
    9    // If ReallyImportantQuery is active, it will be passed to onQueryUpdated.
    10    // If no query with that name is active, a warning will be logged.
    11  },
    12});

If `ReallyImportantQuery` was already going to be passed to `onQueryUpdated` thanks to your `update` function, then it will only be passed once. Using `refetchQueries: ["ReallyImportantQuery"]` just guarantees the query will be included.

If you find you've included more queries than you expected, you can skip or ignore a query by returning `false` from `onQueryUpdated`, after examining the `ObservableQuery` to determine that it doesn't need refetching. Returning a `Promise` from `onQueryUpdated` causes the final `Promise<FetchResult<TData>>` for the mutation to await any promises returned from `onQueryUpdated`, eliminating the need for the legacy `awaitRefetchQueries: true` option.

To use the `onQueryUpdated` API without performing a mutation, try the [`client.refetchQueries`](https://www.apollographql.com/docs/react/data/refetching/#clientrefetchqueries) method. In the standalone `client.refetchQueries` API, the `refetchQueries: [...]` mutation option is called `include: [...]`, and the `update` function is called `updateCache` for clarity. Otherwise, the same internal system powers both `client.refetchQueries` and refetching queries after a mutation.

## `useMutation` API

Supported options and result fields for the `useMutation` hook are listed below.

Most calls to `useMutation` can omit the majority of these options, but it's useful to know they exist. To learn about the `useMutation` hook API in more detail with usage examples, see the [API reference](https://www.apollographql.com/docs/react/api/react/hooks/).

### Options

The `useMutation` hook accepts the following options:

Operation options

###### [`awaitRefetchQueries` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-awaitrefetchqueries)

`boolean`

If `true`, makes sure all queries included in `refetchQueries` are completed before the mutation is considered complete.

The default value is `false` (queries are refetched asynchronously).

###### [`errorPolicy` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-errorpolicy)

`ErrorPolicy`

Specifies how the mutation handles a response that returns both GraphQL errors and partial results.

For details, see [GraphQL error policies](https://www.apollographql.com/docs/react/data/error-handling/#graphql-error-policies).

The default value is `none`, meaning that the mutation result includes error details but _not_ partial results.

###### [`onCompleted` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-oncompleted)

`(data: MaybeMasked<TData>, clientOptions?: Options<TData, TVariables, TCache>) => void`

A callback function that's called when your mutation successfully completes with zero errors (or if `errorPolicy` is `ignore` and partial data is returned).

This function is passed the mutation's result `data` and any options passed to the mutation.

###### [`onError` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-onerror)

`(error: ErrorLike, clientOptions?: Options<TData, TVariables, TCache>) => void`

A callback function that's called when the mutation encounters one or more errors (unless `errorPolicy` is `ignore`).

This function is passed an [`ApolloError`](https://github.com/apollographql/apollo-client/blob/d96f4578f89b933c281bb775a39503f6cdb59ee8/src/errors/index.ts#L36-L39) object that contains either a `networkError` object or a `graphQLErrors` array, depending on the error(s) that occurred, as well as any options passed the mutation.

###### [`onQueryUpdated` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-onqueryupdated)

`OnQueryUpdated<any>`

Optional callback for intercepting queries whose cache data has been updated by the mutation, as well as any queries specified in the `refetchQueries: [...]` list passed to `client.mutate`.

Returning a `Promise` from `onQueryUpdated` will cause the final mutation `Promise` to await the returned `Promise`. Returning `false` causes the query to be ignored.

###### [`refetchQueries` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-refetchqueries)

`((result: NormalizedExecutionResult<Unmasked<TData>>) => InternalRefetchQueriesInclude) | InternalRefetchQueriesInclude`

An array (or a function that _returns_ an array) that specifies which queries you want to refetch after the mutation occurs.

Each array value can be either:

  * An object containing the `query` to execute, along with any `variables`

  * A string indicating the operation name of the query to refetch




###### [`variables` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-variables)

`Partial<TVariables> & TConfiguredVariables`

An object containing all of the GraphQL variables your mutation requires to execute.

Each key in the object corresponds to a variable name, and that key's value corresponds to the variable value.

Networking options

###### [`client` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-client)

`ApolloClient`

The instance of `ApolloClient` to use to execute the mutation.

By default, the instance that's passed down via context is used, but you can provide a different instance here.

###### [`context` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-context)

`DefaultContext`

If you're using [Apollo Link](https://www.apollographql.com/docs/react/api/link/introduction/), this object is the initial value of the `context` object that's passed along your link chain.

###### [`notifyOnNetworkStatusChange` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-notifyonnetworkstatuschange)

`boolean`

If `true`, the in-progress mutation's associated component re-renders whenever the network status changes or a network error occurs.

The default value is `true`.

Caching options

###### [`fetchPolicy` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-fetchpolicy)

`MutationFetchPolicy`

Provide `no-cache` if the mutation's result should _not_ be written to the Apollo Client cache.

The default value is `network-only` (which means the result _is_ written to the cache).

Unlike queries, mutations _do not_ support [fetch policies](https://www.apollographql.com/docs/react/data/queries/#setting-a-fetch-policy) besides `network-only` and `no-cache`.

###### [`optimisticResponse` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-optimisticresponse)

`Unmasked<NoInfer<TData>> | ((vars: TVariables, { IGNORE }: { IGNORE: IgnoreModifier; }) => Unmasked<NoInfer<TData>> | IgnoreModifier)`

By providing either an object or a callback function that, when invoked after a mutation, allows you to return optimistic data and optionally skip updates via the `IGNORE` sentinel object, Apollo Client caches this temporary (and potentially incorrect) response until the mutation completes, enabling more responsive UI updates.

For more information, see [Optimistic mutation results](https://www.apollographql.com/docs/react/performance/optimistic-ui/).

###### [`update` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-update)

`MutationUpdaterFunction<TData, TVariables, TCache>`

A function used to update the Apollo Client cache after the mutation completes.

For more information, see [Updating the cache after a mutation](https://www.apollographql.com/docs/react/data/mutations#updating-the-cache-after-a-mutation).

Other

###### [`keepRootFields` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-keeprootfields)

`boolean`

To avoid retaining sensitive information from mutation root field arguments, Apollo Client v3.4+ automatically clears any `ROOT_MUTATION` fields from the cache after each mutation finishes. If you need this information to remain in the cache, you can prevent the removal by passing `keepRootFields: true` to the mutation. `ROOT_MUTATION` result data are also passed to the mutation `update` function, so we recommend obtaining the results that way, rather than using this option, if possible.

###### [`updateQueries` _(optional)_](https://www.apollographql.com/docs/react/data/mutations#mutationhookoptions-interface-updatequeries)

`MutationQueryReducersMap<TData>`

A `MutationQueryReducersMap`, which is map from query names to mutation query reducers. Briefly, this map defines how to incorporate the results of the mutation into the results of queries that are currently being watched by your application.

### Result

The `useMutation` result is a tuple with a mutate function in the first position and an object representing the mutation result in the second position.

You call the mutate function to trigger the mutation from your UI.

###### [`called`](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-called)

`boolean`

If `true`, the mutation's mutate function has been called.

###### [`client`](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-client)

`ApolloClient`

The instance of Apollo Client that executed the mutation.

Can be useful for manually executing followup operations or writing data to the cache.

###### [`data`](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-data)

`MaybeMasked<TData> | null | undefined`

The data returned from your mutation. Can be `undefined` if the `errorPolicy` is `all` or `ignore` and the server returns a GraphQL response with `errors` but not `data` or a network error is returned.

###### [`error`](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-error)

`ErrorLike | undefined`

If the mutation produces one or more errors, this object contains either an array of `graphQLErrors` or a single `networkError`. Otherwise, this value is `undefined`.

For more information, see [Handling operation errors](https://www.apollographql.com/docs/react/data/error-handling/).

###### [`loading`](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-loading)

`boolean`

If `true`, the mutation is currently in flight.

###### [`reset`](https://www.apollographql.com/docs/react/data/mutations#mutationresult-interface-reset)

`() => void`

A function that you can call to reset the mutation's result to its initial, uncalled state.

## Next steps

The `useQuery` and `useMutation` hooks together represent Apollo Client's core API for performing GraphQL operations. Now that you're familiar with both, you can begin to take advantage of Apollo Client's full feature set, including:

  * [Optimistic UI](https://www.apollographql.com/docs/react/performance/optimistic-ui/): Learn how to improve perceived performance by returning an optimistic response before your mutation result comes back from the server.

  * [Local state](https://www.apollographql.com/docs/react/local-state/local-state-management/): Use Apollo Client to manage the entirety of your application's local state by executing client-side mutations.

  * [Caching in Apollo](https://www.apollographql.com/docs/react/caching/cache-configuration/): Dive deep into the Apollo Client cache and how it's normalized. Understanding the cache is helpful when writing `update` functions for your mutations!




[PreviousFragments](/docs/react/data/fragments)

[NextTypeScript with Apollo Client](/docs/react/data/typescript)

Give FeedbackFeedback

[ Edit on GitHub ](https://github.com/apollographql/apollo-client/blob/main/./docs/source/data/mutations.mdx) [ Ask Community ](https://community.apollographql.com/)

[ ](https://www.apollographql.com)

(C) 2025 Apollo Graph Inc., d/b/a Apollo GraphQL. 

[ Privacy Policy ](https://www.apollographql.com/privacy-policy)

## Company

  * [ About Apollo ](https://www.apollographql.com/about-us)
  * [ Careers ](https://www.apollographql.com/careers)
  * [ Partners ](https://www.apollographql.com/partners)



## Resources

  * [ Blog ](https://blog.apollographql.com)
  * [ Tutorials ](https://www.apollographql.com/tutorials)
  * [ Content Library ](https://www.apollographql.com/resources)



## Get in touch

  * [ Contact Sales ](https://www.apollographql.com/contact-sales)
  * [ Contact Support ](https://www.apollographql.com/support)
