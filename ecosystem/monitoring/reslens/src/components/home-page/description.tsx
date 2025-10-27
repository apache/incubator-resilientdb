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
*
*/

// React and Next.js
import React from "react";

// Layout Components
import { Section, Container } from "@/components/craft";
import Balancer from "react-wrap-balancer";

// Icons
import { Coins } from "lucide-react";

type FeatureText = {
  icon: JSX.Element;
  title: string;
  description: string;
};

const featureText: FeatureText[] = [
  {
    icon: <Coins className="h-6 w-6" />,
    title: "Request",
    description:
      "In the Request phase, a client sends a transaction proposal to the leader node, which verifies and broadcasts the request to all other nodes, initiating the consensus process and waiting for responses.",
  },
  {
    icon: <Coins className="h-6 w-6" />,
    title: "Pre-Prepare / Prepare",
    description:
      "In the Pre-Prepare phase, the primary node proposes a transaction to all replicas, and in the Prepare phase, each replica validates the transaction and broadcasts a 'prepare' message to indicate agreement.",
  },
  {
    icon: <Coins className="h-6 w-6" />,
    title: "Commit",
    description:
      "In the Commit phase, once a quorum of nodes has validated the transaction, the leader node sends a commit message to all nodes, finalizing the transaction and ensuring consistency across the network.",
  },
];

const Description = () => {
  return (
    <Section className="border-b">
      <Container className="not-prose">
        <div className="flex flex-col gap-6">
          <h3 className="text-4xl">
            <Balancer>
              A little to know about PBFT 
            </Balancer>
          </h3>
          <h4 className="text-2xl font-light opacity-70">
            <Balancer>
            PBFT (Practical Byzantine Fault Tolerance) is a consensus algorithm designed to ensure reliable and secure distributed systems, allowing them to reach an agreement even in the presence of faulty or malicious nodes, as long as less than one-third of the nodes are compromised.
            </Balancer>
          </h4>

          <div className="mt-6 grid gap-6 md:mt-12 md:grid-cols-3">
            {featureText.map(({ icon, title, description }, index) => (
              <div className="flex flex-col gap-4" key={index}>
                {icon}
                <h4 className="text-xl text-primary">{title}</h4>
                <p className="text-base opacity-75">{description}</p>
              </div>
            ))}
          </div>
        </div>
      </Container>
    </Section>
  );
};

export default Description;
