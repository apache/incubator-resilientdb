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
*/

"use client";

import { motion } from "framer-motion";
import { useState, useEffect, useCallback } from "react";
import styles from "./DataFlowDiagram.module.css";

const bgSvg = `url('data:image/svg+xml,<svg id="visual" viewBox="0 0 2000 1000" width="2000" height="1000" xmlns="http://www.w3.org/2000/svg"><path d="M0 341L400 241L800 141L1200 301L1600 251L2000 281L2000 0L1600 0L1200 0L800 0L400 0L0 0Z" fill="%230f0f1a"></path><path d="M0 481L400 451L800 311L1200 471L1600 431L2000 461L2000 279L1600 249L1200 299L800 139L400 239L0 339Z" fill="%23161623"></path><path d="M0 581L400 571L800 431L1200 591L1600 551L2000 581L2000 459L1600 429L1200 469L800 309L400 449L0 479Z" fill="%231c1c2d"></path><path d="M0 801L400 751L800 781L1200 831L1600 831L2000 761L2000 579L1600 549L1200 589L800 429L400 569L0 579Z" fill="%231c1c2d"></path><path d="M0 901L400 871L800 871L1200 931L1600 901L2000 831L2000 759L1600 829L1200 829L800 779L400 749L0 799Z" fill="%23161623"></path><path d="M0 1001L400 1001L800 1001L1200 1001L1600 1001L2000 1001L2000 829L1600 899L1200 929L800 869L400 869L0 899Z" fill="%230f0f1a"></path></svg>')`;

const CONSENSUS_ALGORITHMS = [
  ["Pbft", "Geo-Pbft", "PoC"],
  ["HotStuff-1", "RCC", "SpotLess"],
  ["ByShard", "RingBFT", "FairDAG"],
  ["Thunderbolt", "Cerberus", "Trust-BFT"],
  ["Fides", "Cassandra"],
];

type ToolIconType =
  | "beacon"
  | "mcp"
  | "nexus"
  | "pencil"
  | "document"
  | "terminal"
  | "target"
  | "arrowCircle"
  | "search"
  | "ansible"
  | "orbit"
  | "eye"
  | "flame"
  | "chart"
  | "plus";

type GroupTheme = "genAi" | "service" | "monitoring";

interface EcosystemTool {
  name: string;
  icon: ToolIconType;
}

const GEN_AI_TOOLS: EcosystemTool[] = [
  { name: "Beacon", icon: "beacon" },
  { name: "All MCP Servers", icon: "mcp" },
  { name: "Nexus", icon: "nexus" },
  { name: "Drawing-lib", icon: "pencil" },
];

const SERVICE_TOOLS: EcosystemTool[] = [
  { name: "ResContract", icon: "document" },
  { name: "KV Service CLI", icon: "terminal" },
  { name: "UTXO Client Tools", icon: "target" },
  { name: "GraphQL Gateway Tools", icon: "arrowCircle" },
];

const MONITORING_TOOLS: EcosystemTool[] = [
  { name: "ResLens", icon: "search" },
  { name: "Ansible", icon: "ansible" },
  { name: "Orbit", icon: "orbit" },
  { name: "DeepObserve", icon: "eye" },
  { name: "Prometheus", icon: "flame" },
  { name: "Grafana", icon: "chart" },
  { name: "Healthcheck Scripts", icon: "plus" },
];

const GROUP_THEME_CLASS: Record<GroupTheme, string> = {
  genAi: styles.ecosystemGroupGenAi,
  service: styles.ecosystemGroupService,
  monitoring: styles.ecosystemGroupMonitoring,
};

const ToolIcon = ({ type }: { type: ToolIconType }) => {
  const props = { className: styles.toolIconSvg, viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: 1.75, strokeLinecap: "round" as const, strokeLinejoin: "round" as const };

  switch (type) {
    case "beacon":
      return (
        <svg {...props}>
          <rect x="5" y="5" width="14" height="14" rx="2" />
          <path d="M12 15V9M9 12l3-3 3 3" />
        </svg>
      );
    case "mcp":
      return (
        <svg {...props}>
          <circle cx="6" cy="6" r="2" />
          <circle cx="18" cy="6" r="2" />
          <circle cx="12" cy="18" r="2" />
          <path d="M7.5 7.5l3.5 8M16.5 7.5l-3.5 8M8 6h8" />
        </svg>
      );
    case "nexus":
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="9" />
          <ellipse cx="12" cy="12" rx="9" ry="3.5" />
          <path d="M12 3v18M3 12h18" />
        </svg>
      );
    case "pencil":
      return (
        <svg {...props}>
          <path d="M16.5 3.5a2.12 2.12 0 013 3L7 19l-4 1 1-4 12.5-12.5z" />
        </svg>
      );
    case "document":
      return (
        <svg {...props}>
          <path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8z" />
          <path d="M14 2v6h6M9 13h6M9 17h4" />
        </svg>
      );
    case "terminal":
      return (
        <svg {...props}>
          <rect x="3" y="4" width="18" height="16" rx="2" />
          <path d="M7 9l3 3-3 3M12 15h5" />
        </svg>
      );
    case "target":
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="8" />
          <circle cx="12" cy="12" r="4" />
          <circle cx="12" cy="12" r="1" fill="currentColor" stroke="none" />
        </svg>
      );
    case "arrowCircle":
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="9" />
          <path d="M10 12h6M14 10l2 2-2 2" />
        </svg>
      );
    case "search":
      return (
        <svg {...props}>
          <circle cx="11" cy="11" r="7" />
          <path d="M20 20l-3.5-3.5" />
        </svg>
      );
    case "ansible":
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="9" />
          <path d="M8 16l4-10 4 10M9.5 12.5h5" strokeWidth={2} />
        </svg>
      );
    case "orbit":
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="4" />
          <ellipse cx="12" cy="12" rx="9" ry="3" />
        </svg>
      );
    case "eye":
      return (
        <svg {...props}>
          <path d="M2 12s4-7 10-7 10 7 10 7-4 7-10 7-10-7-10-7z" />
          <circle cx="12" cy="12" r="2.5" />
        </svg>
      );
    case "flame":
      return (
        <svg {...props}>
          <path d="M12 3c-1 3-4 4-4 8a4 4 0 008 0c0-4-3-5-4-8z" />
          <path d="M12 11v6" />
        </svg>
      );
    case "chart":
      return (
        <svg {...props}>
          <path d="M6 20V10M12 20V4M18 20v-8" strokeWidth={2.25} />
        </svg>
      );
    case "plus":
      return (
        <svg {...props}>
          <circle cx="12" cy="12" r="9" />
          <path d="M12 8v8M8 12h8" strokeWidth={2} />
        </svg>
      );
    default:
      return null;
  }
};

const ArrowBidirectional = ({ label }: { label?: string }) => (
  <div className={styles.arrow}>
    <svg width="24" height="48" viewBox="0 0 24 48" fill="none" className={styles.arrowSvg}>
      <path d="M12 4v40" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
      <path d="M12 4l-4 4m4-4l4 4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
      <path d="M12 44l-4-4m4 4l4-4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
    {label && <span className={styles.arrowLabel}>{label}</span>}
  </div>
);

const ArrowRight = () => (
  <div className={styles.arrowHorizontal} aria-hidden>
    <svg width="40" height="24" viewBox="0 0 40 24" fill="none" className={styles.arrowSvg}>
      <path d="M4 12h24" stroke="currentColor" strokeWidth="2" strokeLinecap="round" />
      <path d="M28 8l8 4-8 4" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  </div>
);

const ToolPill = ({ tool }: { tool: EcosystemTool }) => (
  <div className={styles.toolPill}>
    <span className={styles.toolIconBadge} aria-hidden>
      <ToolIcon type={tool.icon} />
    </span>
    <span className={styles.toolName}>{tool.name}</span>
  </div>
);

const EcosystemGroup = ({
  title,
  tools,
  theme,
  active,
  className = "",
}: {
  title: string;
  tools: EcosystemTool[];
  theme: GroupTheme;
  active?: boolean;
  className?: string;
}) => (
  <div
    className={[
      styles.ecosystemGroup,
      GROUP_THEME_CLASS[theme],
      className,
      active ? styles.sectionActive : "",
    ]
      .filter(Boolean)
      .join(" ")}
  >
    <div className={styles.ecosystemGroupTitle}>{title}</div>
    <div className={styles.toolList}>
      {tools.map((tool) => (
        <ToolPill key={tool.name} tool={tool} />
      ))}
    </div>
  </div>
);

export interface DataFlowDiagramProps {
  interval?: number;
  sectionCount?: number;
  className?: string;
  animated?: boolean;
  compact?: boolean;
  fitViewport?: boolean;
}

const DataFlowDiagram = ({
  interval = 1500,
  sectionCount = 6,
  className = "",
  animated = true,
  compact = false,
  fitViewport = false,
}: DataFlowDiagramProps) => {
  const [activeSection, setActiveSection] = useState(0);

  useEffect(() => {
    if (!animated) return;
    const id = setInterval(() => setActiveSection((prev) => (prev + 1) % sectionCount), interval);
    return () => clearInterval(id);
  }, [animated, sectionCount, interval]);

  const isActive = useCallback(
    (section: number) => animated && activeSection === section,
    [animated, activeSection]
  );

  const sectionClass = (section: number) =>
    [styles.section, isActive(section) ? styles.sectionActive : ""].filter(Boolean).join(" ");

  const frameClass = compact ? `${styles.frame} ${styles.frameCompact}` : styles.frame;

  const diagram = (
    <motion.div
      initial={{ opacity: 0, scale: 0.96, y: 12 }}
      animate={{ opacity: 1, scale: 1, y: 0 }}
      transition={{ type: "spring", stiffness: 80, damping: 18 }}
      className={frameClass}
      style={{ backgroundImage: bgSvg, backgroundSize: "cover", backgroundPosition: "center" }}
    >
      <div className={styles.architectureLayout}>
        <div className={styles.ecosystemHeader}>Ecosystem Tools</div>

        <EcosystemGroup
          title="Gen AI Tools"
          tools={GEN_AI_TOOLS}
          theme="genAi"
          active={isActive(0)}
          className={styles.ecosystemRowClient}
        />
        <div className={styles.arrowRowClient}>
          <ArrowRight />
        </div>
        <motion.div
          initial={{ y: -10, opacity: 0 }}
          animate={{ y: 0, opacity: 1 }}
          transition={{ type: "spring", stiffness: 90, damping: 18 }}
          className={`${sectionClass(0)} ${styles.mainRowClient}`}
        >
          <div className={styles.sectionHeader}>
                <span>Client Interfaces</span>
              </div>
              <div className={styles.grid3}>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardSky} ${styles.flexCenter}`} style={{ padding: "0.75rem 1rem" }}>
                  <div className={`${styles.textSky} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>SDKs</div>
                  <div className={styles.textSkyLight} style={{ fontSize: "0.875rem" }}>C++ · Python · Go</div>
                </motion.div>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardIndigo} ${styles.flexCenter}`} style={{ padding: "0.75rem 1rem" }}>
                  <div className={`${styles.textIndigo} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>RESTful APIs</div>
                  <div className={styles.textIndigoLight} style={{ fontSize: "0.875rem" }}>HTTP · JSON</div>
                </motion.div>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardEmerald} ${styles.flexCenter}`} style={{ padding: "0.75rem 1rem" }}>
                  <div className={`${styles.textEmerald} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>GraphQL</div>
                  <div className={styles.textEmeraldLight} style={{ fontSize: "0.875rem" }}>Queries · Mutations</div>
                </motion.div>
              </div>
        </motion.div>

        <div className={styles.connectorRpc}>
          <ArrowBidirectional label="RPC" />
        </div>

        <EcosystemGroup
          title="Service-Specific Tools"
          tools={SERVICE_TOOLS}
          theme="service"
          active={isActive(1)}
          className={styles.ecosystemRowService}
        />
        <div className={styles.arrowRowService}>
          <ArrowRight />
        </div>
        <motion.div
          initial={{ y: -16, opacity: 0 }}
          animate={{ y: 0, opacity: 1 }}
          transition={{ delay: 0.05, type: "spring", stiffness: 90, damping: 18 }}
          className={`${sectionClass(1)} ${styles.mainRowService}`}
        >
              <div className={styles.sectionHeader}>
                <span>Service Layer</span>
              </div>
              <div className={styles.grid3}>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardSky} ${styles.flexCenter}`} style={{ padding: "0.75rem 1rem" }}>
                  <div className={`${styles.textSky} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>Key-Value</div>
                  <div className={styles.textSkyLight} style={{ fontSize: "0.875rem" }}>KV Store</div>
                </motion.div>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardIndigo} ${styles.flexCenter}`} style={{ padding: "0.75rem 1rem" }}>
                  <div className={`${styles.textIndigo} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>Smart Contract</div>
                  <div className={styles.textIndigoLight} style={{ fontSize: "0.875rem" }}>EVM · Solidity</div>
                </motion.div>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardEmerald} ${styles.flexCenter}`} style={{ padding: "0.75rem 1rem" }}>
                  <div className={`${styles.textEmerald} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>UTXO</div>
                  <div className={styles.textEmeraldLight} style={{ fontSize: "0.875rem" }}>Unspent Outputs</div>
                </motion.div>
              </div>
        </motion.div>

        <div className={styles.connectorFc}>
          <ArrowBidirectional label="Function Calling" />
        </div>

        <EcosystemGroup
          title="Monitoring Tools"
          tools={MONITORING_TOOLS}
          theme="monitoring"
          active={isActive(2) || isActive(4) || isActive(5)}
          className={styles.ecosystemRowExec}
        />
        <div className={styles.arrowRowExec}>
          <ArrowRight />
        </div>
        <motion.div
          initial={{ scale: 0.96, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          transition={{ delay: 0.1, type: "spring", stiffness: 85, damping: 18 }}
          className={`${styles.section} ${styles.mainRowExec} ${isActive(2) || isActive(4) || isActive(5) ? styles.sectionActive : ""}`}
        >
              <div className={styles.sectionHeader}>
                <span>Execution Layer</span>
              </div>

              <motion.div
                whileHover={{ scale: 1.005 }}
                className={`${styles.cardBase} ${styles.cardTealLight} ${isActive(2) ? styles.sectionActive : ""}`}
                style={{ padding: "1rem", marginBottom: "1rem" }}
              >
                <div className={`${styles.textTeal} ${styles.mb2}`} style={{ fontSize: "0.75rem", textTransform: "uppercase", letterSpacing: "0.16em" }}>
                  Consensus & Ordering
                </div>
                <div className={styles.consensusGrid}>
                  {CONSENSUS_ALGORITHMS.map((row, rowIdx) => (
                    <div key={rowIdx} className={styles.consensusRow}>
                      {row.map((algo) => (
                        <div key={algo} className={`${styles.smallCard} ${styles.cardSlateDark} ${styles.textSlate} ${styles.consensusAlgo}`}>
                          {algo}
                        </div>
                      ))}
                    </div>
                  ))}
                </div>
                <div className={`${styles.smallCard} ${styles.cardCyanBold} ${styles.networkSubstrate}`}>
                  Network Substrate — TCP Messaging
                </div>
              </motion.div>

              <div className={styles.gap5} style={{ display: "grid" }}>
                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardEmeraldLight} ${isActive(4) ? styles.sectionActive : ""}`} style={{ padding: "1rem" }}>
                  <div className={styles.sectionHeader} style={{ color: "rgba(167, 243, 208, 0.9)" }}>
                    <span>Transaction Execution</span>
                  </div>
                  <div className={styles.grid3} style={{ fontSize: "0.875rem" }}>
                    <div className={`${styles.executorCard} ${styles.executorCardTeal}`}>
                      <svg className={`${styles.iconSm} ${styles.iconTeal}`} fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 1121 9z" />
                      </svg>
                      <span style={{ fontWeight: 500 }}>Key-Value Executor</span>
                    </div>
                    <div className={`${styles.executorCard} ${styles.executorCardIndigo}`}>
                      <svg className={`${styles.iconSm} ${styles.iconIndigo}`} fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10 20l4-16m4 4l4 4-4 4M6 16l-4-4 4-4" />
                      </svg>
                      <span style={{ fontWeight: 500 }}>Smart Contract Executor</span>
                    </div>
                    <div className={`${styles.executorCard} ${styles.executorCardEmerald}`}>
                      <svg className={`${styles.iconSm} ${styles.iconEmerald}`} fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                      </svg>
                      <span style={{ fontWeight: 500 }}>UTXO Executor</span>
                    </div>
                  </div>
                </motion.div>

                <motion.div whileHover={{ scale: 1.01 }} className={`${styles.cardBase} ${styles.cardSky} ${isActive(5) ? styles.sectionActive : ""}`} style={{ padding: "1rem" }}>
                  <div className={styles.sectionHeader} style={{ marginBottom: "0.5rem", color: "rgba(186, 230, 253, 0.9)" }}>
                    <span>Blockchain State & Storage</span>
                  </div>
                  <div style={{ display: "grid", gap: "0.75rem" }}>
                    <div className={`${styles.smallCard} ${styles.cardCyanBold}`} style={{ padding: "0.5rem 0.75rem", textAlign: "center", fontSize: "0.875rem", color: "rgb(236, 254, 255)" }}>
                      High-Throughput In-Memory Chain State
                    </div>
                    <div className={styles.grid2} style={{ fontSize: "0.875rem" }}>
                      <div className={`${styles.smallCard} ${styles.cardSlateDarker}`} style={{ padding: "0.625rem 0.75rem", display: "flex", alignItems: "center", gap: "0.5rem", color: "rgb(240, 249, 255)" }}>
                        <svg className={`${styles.iconSm} ${styles.iconCyan}`} fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 7v10c0 2.21 3.582 4 8 4s8-1.79 8-4V7M4 7c0 2.21 3.582 4 8 4s8-1.79 8-4M4 7c0-2.21 3.582-4 8-4s8 1.79 8 4m0 5c0 2.21-3.582 4-8 4s-8-1.79-8-4" />
                        </svg>
                        <div>
                          <div style={{ fontWeight: 500 }}>Embeddable KV Store</div>
                          <div style={{ fontSize: "10px", color: "rgba(186, 230, 253, 0.8)" }}>LevelDB</div>
                        </div>
                      </div>
                      <div className={`${styles.smallCard} ${styles.cardSlateDarker}`} style={{ padding: "0.625rem 0.75rem", display: "flex", alignItems: "center", gap: "0.5rem", color: "rgb(240, 249, 255)" }}>
                        <svg className={`${styles.iconSm} ${styles.iconCyan}`} fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z" />
                        </svg>
                        <div>
                          <div style={{ fontWeight: 500 }}>Memory DB</div>
                          <div style={{ fontSize: "10px", color: "rgba(186, 230, 253, 0.8)" }}>In-Memory Store</div>
                        </div>
                      </div>
                    </div>
                  </div>
                </motion.div>
              </div>
        </motion.div>
      </div>
    </motion.div>
  );

  const wrapperClass = [styles.wrapper, compact ? styles.wrapperCompact : "", className].filter(Boolean).join(" ");

  if (fitViewport && compact) {
    return (
      <div className={`${wrapperClass} ${styles.fitWrapper}`}>
        <div className={styles.fitInner} style={{ transform: "scale(calc(clamp(0.55, (100vh - 200px) / 520, 1)))" }}>
          {diagram}
        </div>
      </div>
    );
  }

  return <div className={wrapperClass}>{diagram}</div>;
};

export default DataFlowDiagram;
