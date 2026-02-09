# Licensed to the Apache Software Foundation (ASF) under one
  # or more contributor license agreements.  See the NOTICE file
  # distributed with this work for additional information
  # regarding copyright ownership.  The ASF licenses this file
  # to you under the Apache License, Version 2.0 (the
  # "License"); you may not use this file except in compliance
  # with the License.  You may obtain a copy of the License at
  #
  #   http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing,
  # software distributed under the License is distributed on an
  # "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  # KIND, either express or implied.  See the License for the
  # specific language governing permissions and limitations
  # under the License.

"""
Knowledge Graph Builder Module
Creates visual concept maps showing relationships between distributed systems concepts
"""

import asyncio
from typing import Dict, Any, List, Set, Tuple, Optional
import logging

logger = logging.getLogger("KnowledgeGraph")


class KnowledgeGraphBuilder:
    """Builds and manages knowledge graphs for distributed systems concepts"""
    
    def __init__(self):
        self.graph = self._build_resilientdb_knowledge_graph()
    
    def _build_resilientdb_knowledge_graph(self) -> Dict[str, Any]:
        """Build comprehensive knowledge graph for ResilientDB and distributed systems"""
        
        # Define nodes (concepts) and their metadata
        nodes = {
            # Fundamentals
            "Distributed Systems": {
                "category": "fundamentals",
                "description": "Systems with components on networked computers",
                "prerequisites": [],
                "difficulty": "beginner"
            },
            "CAP Theorem": {
                "category": "fundamentals",
                "description": "Consistency, Availability, Partition tolerance tradeoffs",
                "prerequisites": ["Distributed Systems"],
                "difficulty": "beginner"
            },
            "Consensus": {
                "category": "fundamentals",
                "description": "Agreement among distributed processes",
                "prerequisites": ["Distributed Systems"],
                "difficulty": "intermediate"
            },
            
            # Byzantine Fault Tolerance
            "Byzantine Fault Tolerance": {
                "category": "bft",
                "description": "Tolerating arbitrary node failures including malicious behavior",
                "prerequisites": ["Distributed Systems", "Consensus"],
                "difficulty": "intermediate"
            },
            "Byzantine Generals Problem": {
                "category": "bft",
                "description": "Classic problem of achieving agreement with traitors",
                "prerequisites": ["Byzantine Fault Tolerance"],
                "difficulty": "intermediate"
            },
            "PBFT": {
                "category": "bft",
                "description": "Practical Byzantine Fault Tolerance algorithm",
                "prerequisites": ["Byzantine Fault Tolerance", "Byzantine Generals Problem"],
                "difficulty": "advanced"
            },
            "Quorum": {
                "category": "bft",
                "description": "Minimum number of nodes needed for agreement (2f+1)",
                "prerequisites": ["Byzantine Fault Tolerance"],
                "difficulty": "intermediate"
            },
            
            # ResilientDB Core
            "ResilientDB": {
                "category": "resilientdb",
                "description": "High-performance BFT database system",
                "prerequisites": ["PBFT", "Consensus"],
                "difficulty": "intermediate"
            },
            "ResilientDB Architecture": {
                "category": "resilientdb",
                "description": "Modular architecture with pipeline consensus",
                "prerequisites": ["ResilientDB"],
                "difficulty": "intermediate"
            },
            "Pipeline Consensus": {
                "category": "resilientdb",
                "description": "Overlapping consensus phases for high throughput",
                "prerequisites": ["PBFT", "ResilientDB"],
                "difficulty": "advanced"
            },
            "Transaction Processing": {
                "category": "resilientdb",
                "description": "How ResilientDB processes and orders transactions",
                "prerequisites": ["ResilientDB Architecture"],
                "difficulty": "intermediate"
            },
            
            # GraphQL
            "GraphQL": {
                "category": "api",
                "description": "Query language for APIs",
                "prerequisites": [],
                "difficulty": "beginner"
            },
            "GraphQL Schema": {
                "category": "api",
                "description": "Type system defining API structure",
                "prerequisites": ["GraphQL"],
                "difficulty": "beginner"
            },
            "GraphQL Queries": {
                "category": "api",
                "description": "Reading data from GraphQL API",
                "prerequisites": ["GraphQL Schema"],
                "difficulty": "beginner"
            },
            "GraphQL Mutations": {
                "category": "api",
                "description": "Writing data to GraphQL API",
                "prerequisites": ["GraphQL Schema"],
                "difficulty": "beginner"
            },
            "ResilientDB GraphQL": {
                "category": "resilientdb",
                "description": "GraphQL interface for ResilientDB",
                "prerequisites": ["ResilientDB", "GraphQL Schema"],
                "difficulty": "intermediate"
            },
            
            # Applications
            "Debitable": {
                "category": "applications",
                "description": "Democratic social media platform",
                "prerequisites": ["ResilientDB", "Transaction Processing"],
                "difficulty": "advanced"
            },
            "DraftRes": {
                "category": "applications",
                "description": "Provably fair fantasy sports",
                "prerequisites": ["ResilientDB", "Transaction Processing"],
                "difficulty": "advanced"
            },
            "Arrayán": {
                "category": "applications",
                "description": "Supply chain traceability platform",
                "prerequisites": ["ResilientDB", "Transaction Processing"],
                "difficulty": "advanced"
            },
            
            # Advanced Topics
            "Performance Optimization": {
                "category": "advanced",
                "description": "Tuning ResilientDB for maximum throughput",
                "prerequisites": ["ResilientDB Architecture", "Pipeline Consensus"],
                "difficulty": "advanced"
            },
            "View Changes": {
                "category": "advanced",
                "description": "Recovering from primary node failures",
                "prerequisites": ["PBFT"],
                "difficulty": "advanced"
            },
            "Checkpointing": {
                "category": "advanced",
                "description": "Periodic state snapshots for recovery",
                "prerequisites": ["PBFT", "ResilientDB Architecture"],
                "difficulty": "advanced"
            }
        }
        
        # Define edges (relationships)
        edges = []
        for node_name, node_data in nodes.items():
            for prereq in node_data["prerequisites"]:
                edges.append({
                    "from": prereq,
                    "to": node_name,
                    "type": "prerequisite"
                })
        
        # Add semantic relationships
        semantic_edges = [
            ("ResilientDB", "Debitable", "implements"),
            ("ResilientDB", "DraftRes", "implements"),
            ("ResilientDB", "Arrayán", "implements"),
            ("PBFT", "Pipeline Consensus", "optimized_by"),
            ("GraphQL", "ResilientDB GraphQL", "interface_for"),
            ("Transaction Processing", "ResilientDB GraphQL", "exposes"),
            ("CAP Theorem", "Byzantine Fault Tolerance", "informs"),
            ("Quorum", "PBFT", "used_in"),
        ]
        
        for from_node, to_node, rel_type in semantic_edges:
            edges.append({
                "from": from_node,
                "to": to_node,
                "type": rel_type
            })
        
        return {"nodes": nodes, "edges": edges}
    
    async def build_graph(self, topic: str, depth: int = 2) -> Dict[str, Any]:
        """Build knowledge graph centered on a topic"""
        
        # Find the topic node
        if topic not in self.graph["nodes"]:
            # Try partial match
            matches = [n for n in self.graph["nodes"].keys() if topic.lower() in n.lower()]
            if matches:
                topic = matches[0]
            else:
                return {"error": f"Topic '{topic}' not found in knowledge graph"}
        
        # BFS to find connected nodes within depth
        visited_nodes = set()
        current_level = {topic}
        
        for _ in range(depth):
            next_level = set()
            for node in current_level:
                visited_nodes.add(node)
                # Find connected nodes
                for edge in self.graph["edges"]:
                    if edge["from"] == node:
                        next_level.add(edge["to"])
                    elif edge["to"] == node:
                        next_level.add(edge["from"])
            current_level = next_level - visited_nodes
        
        # Build subgraph
        subgraph_nodes = {k: v for k, v in self.graph["nodes"].items() if k in visited_nodes}
        subgraph_edges = [e for e in self.graph["edges"] 
                         if e["from"] in visited_nodes and e["to"] in visited_nodes]
        
        # Generate Mermaid diagram
        mermaid = self._generate_mermaid_diagram(subgraph_nodes, subgraph_edges, topic)
        
        # Generate text representation
        text_rep = self._generate_text_representation(subgraph_nodes, subgraph_edges, topic)
        
        return {
            "total_nodes": len(subgraph_nodes),
            "total_edges": len(subgraph_edges),
            "center_topic": topic,
            "mermaid_diagram": mermaid,
            "text_representation": text_rep,
            "nodes": subgraph_nodes,
            "edges": subgraph_edges
        }
    
    def _generate_mermaid_diagram(
        self,
        nodes: Dict[str, Any],
        edges: List[Dict[str, str]],
        center: str
    ) -> str:
        """Generate Mermaid flowchart diagram"""
        
        mermaid = "graph TD\n"
        
        # Add nodes with styling based on category
        for node_name, node_data in nodes.items():
            node_id = node_name.replace(" ", "_").replace("-", "_")
            category = node_data["category"]
            
            # Style based on category
            if node_name == center:
                style = ":::highlight"
            elif category == "fundamentals":
                style = ":::fundamental"
            elif category == "bft":
                style = ":::bft"
            elif category == "resilientdb":
                style = ":::resilientdb"
            elif category == "applications":
                style = ":::application"
            else:
                style = ""
            
            mermaid += f'    {node_id}["{node_name}"]{style}\n'
        
        # Add edges
        for edge in edges:
            from_id = edge["from"].replace(" ", "_").replace("-", "_")
            to_id = edge["to"].replace(" ", "_").replace("-", "_")
            edge_type = edge["type"]
            
            if edge_type == "prerequisite":
                arrow = "-->|prereq|"
            elif edge_type == "implements":
                arrow = "-.->|implements|"
            elif edge_type == "optimized_by":
                arrow = "==>|optimizes|"
            else:
                arrow = f"-->|{edge_type}|"
            
            mermaid += f"    {from_id} {arrow} {to_id}\n"
        
        # Add styling
        mermaid += "\n    classDef highlight fill:#f96,stroke:#333,stroke-width:4px\n"
        mermaid += "    classDef fundamental fill:#9cf,stroke:#333,stroke-width:2px\n"
        mermaid += "    classDef bft fill:#fc9,stroke:#333,stroke-width:2px\n"
        mermaid += "    classDef resilientdb fill:#9f9,stroke:#333,stroke-width:2px\n"
        mermaid += "    classDef application fill:#f9f,stroke:#333,stroke-width:2px\n"
        
        return mermaid
    
    def _generate_text_representation(
        self,
        nodes: Dict[str, Any],
        edges: List[Dict[str, str]],
        center: str
    ) -> str:
        """Generate text-based graph representation"""
        
        text = f"📊 Knowledge Graph: {center}\n\n"
        
        # Group by category
        categories = {}
        for node_name, node_data in nodes.items():
            cat = node_data["category"]
            if cat not in categories:
                categories[cat] = []
            categories[cat].append(node_name)
        
        for category, node_list in categories.items():
            text += f"**{category.title()}:**\n"
            for node in node_list:
                marker = "🎯" if node == center else "•"
                text += f"  {marker} {node}\n"
            text += "\n"
        
        return text
    
    async def get_related_concepts(self, concept: str) -> List[Dict[str, str]]:
        """Get concepts directly related to a given concept"""
        
        if concept not in self.graph["nodes"]:
            # Try partial match
            matches = [n for n in self.graph["nodes"].keys() if concept.lower() in n.lower()]
            if matches:
                concept = matches[0]
            else:
                return []
        
        related = []
        
        # Find all edges involving this concept
        for edge in self.graph["edges"]:
            if edge["from"] == concept:
                related.append({
                    "name": edge["to"],
                    "relationship": f"is {edge['type']} of",
                    "direction": "forward"
                })
            elif edge["to"] == concept:
                related.append({
                    "name": edge["from"],
                    "relationship": f"{edge['type']} for",
                    "direction": "backward"
                })
        
        return related
    
    async def find_learning_path(
        self,
        from_concept: str,
        to_concept: str,
        student_mastery: List[str]
    ) -> Dict[str, Any]:
        """Find optimal learning path between concepts"""
        
        # BFS to find shortest path
        queue = [(from_concept, [from_concept])]
        visited = {from_concept}
        
        while queue:
            current, path = queue.pop(0)
            
            if current == to_concept:
                # Found path, build detailed steps
                steps = []
                total_time = 0
                
                for concept in path:
                    node_data = self.graph["nodes"].get(concept, {})
                    is_mastered = concept in student_mastery
                    
                    # Estimate time based on difficulty
                    difficulty = node_data.get("difficulty", "intermediate")
                    time_map = {"beginner": "30 min", "intermediate": "1 hour", "advanced": "2 hours"}
                    
                    steps.append({
                        "concept": concept,
                        "description": node_data.get("description", ""),
                        "difficulty": difficulty,
                        "estimated_time": time_map.get(difficulty, "1 hour"),
                        "mastered": is_mastered
                    })
                    
                    if not is_mastered:
                        total_time += {"beginner": 30, "intermediate": 60, "advanced": 120}.get(difficulty, 60)
                
                return {
                    "steps": steps,
                    "total_time": f"{total_time // 60} hours {total_time % 60} min",
                    "path_length": len(path)
                }
            
            # Explore neighbors
            for edge in self.graph["edges"]:
                next_node = None
                if edge["from"] == current and edge["type"] == "prerequisite":
                    next_node = edge["to"]
                
                if next_node and next_node not in visited:
                    visited.add(next_node)
                    queue.append((next_node, path + [next_node]))
        
        return {
            "error": f"No learning path found from {from_concept} to {to_concept}",
            "steps": [],
            "total_time": "N/A"
        }
    
    async def export_full_graph(self) -> str:
        """Export complete knowledge graph"""
        
        total_nodes = len(self.graph["nodes"])
        total_edges = len(self.graph["edges"])
        
        # Generate full mermaid diagram
        mermaid = self._generate_mermaid_diagram(
            self.graph["nodes"],
            self.graph["edges"],
            "ResilientDB"
        )
        
        return f"""
# Complete ResilientDB Knowledge Graph

**Statistics:**
- Total Concepts: {total_nodes}
- Total Relationships: {total_edges}

**Categories:**
{self._list_categories()}

**Full Graph:**
```mermaid
{mermaid}
```

**All Concepts:**
{self._list_all_concepts()}
        """
    
    def _list_categories(self) -> str:
        """List all categories with counts"""
        categories = {}
        for node_data in self.graph["nodes"].values():
            cat = node_data["category"]
            categories[cat] = categories.get(cat, 0) + 1
        
        return "\n".join(f"- {cat.title()}: {count} concepts" 
                        for cat, count in sorted(categories.items()))
    
    def _list_all_concepts(self) -> str:
        """List all concepts grouped by difficulty"""
        by_difficulty = {"beginner": [], "intermediate": [], "advanced": []}
        
        for node_name, node_data in self.graph["nodes"].items():
            difficulty = node_data.get("difficulty", "intermediate")
            by_difficulty[difficulty].append(node_name)
        
        result = ""
        for level in ["beginner", "intermediate", "advanced"]:
            result += f"\n**{level.title()}:**\n"
            for concept in sorted(by_difficulty[level]):
                result += f"- {concept}\n"
        
        return result
