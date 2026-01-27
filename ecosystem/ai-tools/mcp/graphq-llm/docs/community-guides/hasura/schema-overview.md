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
description: Manage schema on your database with Hasura
keywords:
  - hasura
  - docs
  - schema
title: GraphQL Schema Overview
sidebar_label: Overview
hide_table_of_contents: true
sidebar_position: 1
---

import Thumbnail from '@site/src/components/Thumbnail';
import VersionedLink from '@site/src/components/VersionedLink';
import Schema from '@site/static/icons/features/schema.svg';

# <Schema /> GraphQL Schema

<div className="overview-header">
  <div className="overview-text">
    <p>
      Hasura's GraphQL Engine can be used to build a flexible and scalable GraphQL API on top of your existing Postgres,
      MySQL, Microsoft SQL Server, Athena, Snowflake, BigQuery, or Oracle database.
    </p>
    <p>
      The Hasura GraphQL Engine automatically generates a GraphQL schema based on the tables and views in your database.{' '}
      <b>You no longer need to write a GraphQL schema, endpoints, or resolvers</b>.
    </p>
    <p>
      The Hasura GraphQL Engine lets you do anything you would usually do with your database by giving you GraphQL over
      native constructs.
    </p>
    <h4>Quick Links</h4>
    <ul>
      <li>
        <VersionedLink to="/schema/quickstart">
          Create tables on a connected data source in less than 30 seconds.
        </VersionedLink>
      </li>
      <li>
        <VersionedLink to="/getting-started/how-it-works/index">Learn more about how Hasura works.</VersionedLink>
      </li>
    </ul>
  </div>
  <iframe
    src="https://www.youtube.com/embed/zV7y7XTQZ3w"
    frameBorder="0"
    allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture"
    allowFullScreen
  />
</div>

## Understanding the GraphQL Schema

<div className="overview-gallery">
  <VersionedLink to="/schema/postgres/postgres-guides/import-data-from-csv/">
    <div className="card">
      <h3>Import data from a CSV</h3>
      <p>
        Learn how to quickly and easily import data from a CSV file into a table in your Postgres database using psql.
      </p>
    </div>
  </VersionedLink>
  <VersionedLink to="/schema/postgres/custom-functions/">
    <div className="card">
      <h3>Extend your GraphQL schema with SQL functions</h3>
      <p>
        Learn how to extend your GraphQL schema with SQL functions. This is useful when you want to add custom business
        logic to your GraphQL API.
      </p>
    </div>
  </VersionedLink>
  <VersionedLink to="/schema/postgres/views/">
    <div className="card">
      <h3>Extend your GraphQL schema with Postgres views</h3>
      <p>
        Learn how to extend your GraphQL schema with Postgres views. This is allows you to view the results of a custom
        query as a virtual table.
      </p>
    </div>
  </VersionedLink>
</div>
