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

import { NextResponse } from 'next/server';
import { documentIndexer, DocumentChunk } from '../../../lib/documentIndexer';

interface AISearchResult {
  id: string;
  title: string;
  url: string;
  excerpt: string;
  relevanceScore: number;
  metadata: {
    description?: string;
    parent?: string;
    nav_order?: number;
    tags?: string[];
  };
  matchedContent: string;
  sections: Array<{
    id: string;
    title: string;
    level: number;
    content: string;
    url: string;
  }>;
  matchedSection?: {
    id: string;
    title: string;
    level: number;
    content: string;
    url: string;
  };
}

export async function POST(req: Request) {
  try {
    const { query, limit = 10 } = await req.json();

    if (!query || typeof query !== 'string') {
      return NextResponse.json(
        { error: 'Query is required and must be a string' },
        { status: 400 }
      );
    }

    // Load or create document index
    let index = documentIndexer.loadIndex();
    if (!index || documentIndexer.needsUpdate()) {
      console.log('Index not found or needs update, creating new index...');
      index = await documentIndexer.indexDocuments();
    }

    if (!index || index.documents.length === 0) {
      return NextResponse.json(
        { error: 'No documents found in index' },
        { status: 404 }
      );
    }

    // Use hybrid search (AI + keyword)
    const searchResults = await performHybridSearch(query, index.documents, limit);

    return NextResponse.json({
      results: searchResults,
      totalFound: searchResults.length,
      query,
      timestamp: new Date().toISOString()
    });

  } catch (error) {
    console.error('AI Search API error:', error);
    return NextResponse.json(
      { error: 'Failed to perform AI search' },
      { status: 500 }
    );
  }
}

async function performHybridSearch(
  query: string, 
  documents: DocumentChunk[], 
  limit: number
): Promise<AISearchResult[]> {
  
  // Extract search terms from query
  const searchTerms = query.toLowerCase()
    .replace(/[^\w\s-]/g, ' ')
    .split(/\s+/)
    .filter(word => word.length > 2);
  
  // Get AI-generated search terms
  const aiTerms = await generateSearchTerms(query);
  
  // Combine all search terms
  const allSearchTerms = Array.from(new Set([...searchTerms, ...aiTerms]));
  
  // Get keyword-based scores
  const keywordResults = scoreDocumentsByKeywords(query, searchTerms, documents);
  
  // Get AI-enhanced scores
  const aiResults = scoreDocumentsByAI(query, allSearchTerms, documents);
  
  // Combine scores using weighted average
  const combinedResults = combineSearchResults(keywordResults, aiResults);
  
  // Rank and return top results
  return combinedResults
    .sort((a, b) => b.relevanceScore - a.relevanceScore)
    .slice(0, limit)
    .map(doc => {
      // Find the best matching section
      const matchedSection = findBestMatchingSection(query, allSearchTerms, doc.sections);
      
      return {
        id: doc.id,
        title: doc.title,
        url: matchedSection ? matchedSection.url : doc.url,
        excerpt: doc.excerpt,
        relevanceScore: doc.relevanceScore,
        metadata: doc.metadata,
        matchedContent: doc.matchedContent,
        sections: doc.sections,
        matchedSection
      };
    });
}

function findBestMatchingSection(
  query: string, 
  searchTerms: string[], 
  sections: Array<{
    id: string;
    title: string;
    level: number;
    content: string;
    url: string;
  }>
): any | null {
  if (!sections || sections.length === 0) return null;
  
  let bestSection = null;
  let bestScore = 0;
  
  const queryLower = query.toLowerCase();
  
  for (const section of sections) {
    let score = 0;
    const titleLower = section.title.toLowerCase();
    const contentLower = section.content.toLowerCase();
    
    // Title matching (highest weight)
    if (titleLower.includes(queryLower)) {
      score += 10;
    }
    
    // Search terms in title
    searchTerms.forEach(term => {
      if (titleLower.includes(term.toLowerCase())) {
        score += 5;
      }
    });
    
    // Search terms in content
    searchTerms.forEach(term => {
      const termLower = term.toLowerCase();
      if (contentLower.includes(termLower)) {
        score += 3;
      }
    });
    
    // Word frequency in content
    const queryWords = queryLower.split(/\s+/);
    queryWords.forEach(word => {
      if (word.length > 2) {
        const contentMatches = (contentLower.match(new RegExp(word, 'g')) || []).length;
        score += contentMatches;
      }
    });
    
    // Prefer more specific headings (h2, h3 over h1) for better precision
    // H1 headings are usually too general, prefer H2 and H3 for specific topics
    if (section.level === 1) {
      score -= 2; // Penalize H1 headings as they're usually too general
    } else if (section.level === 2) {
      score += 3; // Prefer H2 headings
    } else if (section.level === 3) {
      score += 2; // Also good for H3 headings
    }
    
    if (score > bestScore) {
      bestScore = score;
      bestSection = section;
    }
  }
  
  // Only return if score is meaningful and we have a good match
  // Require higher scores for H1 headings since they're usually too general
  const minScore = bestSection && bestSection.level === 1 ? 5 : 2;
  return bestScore > minScore ? bestSection : null;
}

// Synonym mapping for better search results
const SYNONYM_MAP: Record<string, string[]> = {
  'caching': ['cache', 'node cache', 'python cache', 'memory', 'storage', 'performance'],
  'cache': ['caching', 'node cache', 'python cache', 'memory', 'storage', 'performance'],
  'storage': ['cache', 'caching', 'database', 'persistence', 'data'],
  'database': ['storage', 'data', 'persistence', 'resilientdb'],
  'consensus': ['agreement', 'pbft', 'consensus management', 'voting'],
  'network': ['communication', 'network communication', 'messaging', 'replica'],
  'transaction': ['txn', 'transaction execution', 'execution', 'processing'],
  'client': ['client interaction', 'kv client', 'utxo client', 'contract client'],
  'configuration': ['config', 'setup', 'resdbconfig', 'settings'],
  'checkpointing': ['checkpoint', 'recovery', 'backup', 'snapshot'],
  'graphql': ['resilientdb graphql', 'api', 'query', 'mutation'],
  'orm': ['resdb orm', 'object relational mapping', 'database orm'],
  'vault': ['resvault', 'secure storage', 'encryption'],
  'lens': ['reslens', 'monitoring', 'observability', 'metrics'],
  'cli': ['rescli', 'command line', 'terminal', 'tools']
};

function expandSearchTerms(query: string): string[] {
  const terms = query.toLowerCase().split(/\s+/);
  const expandedTerms = new Set<string>();
  
  // Add original terms
  terms.forEach(term => expandedTerms.add(term));
  
  // Add synonyms
  terms.forEach(term => {
    if (SYNONYM_MAP[term]) {
      SYNONYM_MAP[term].forEach(synonym => expandedTerms.add(synonym));
    }
  });
  
  // Add partial matches (e.g., "caching" should match "node cache")
  terms.forEach(term => {
    Object.keys(SYNONYM_MAP).forEach(key => {
      if (key.includes(term) || term.includes(key)) {
        SYNONYM_MAP[key].forEach(synonym => expandedTerms.add(synonym));
      }
    });
  });
  
  return Array.from(expandedTerms);
}

async function generateSearchTerms(query: string): Promise<string[]> {
  try {
    // First, expand with synonyms
    const expandedTerms = expandSearchTerms(query);
    
    const response = await fetch('https://api.deepseek.com/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${process.env.DEEPSEEK_API_KEY}`,
      },
      body: JSON.stringify({
        model: 'deepseek-chat',
        messages: [
          {
            role: 'system',
            content: `You are a search term generator for ResilientDB documentation. Given a user query, generate 5-8 relevant search terms that would help find related documentation content.

Focus on:
- Technical terms and concepts
- Related features and components
- Synonyms and alternative terms
- Specific ResilientDB terminology
- Include terms like "node cache", "python cache", "resilientdb graphql", etc.

Return only the search terms, one per line, without any other text.`
          },
          {
            role: 'user',
            content: `Generate search terms for: "${query}"`
          }
        ],
        max_tokens: 200,
        temperature: 0.3
      }),
    });

    if (!response.ok) {
      throw new Error('Failed to generate search terms');
    }

    const data = await response.json();
    const aiTerms = data.choices[0]?.message?.content
      ?.split('\n')
      .map((term: string) => term.trim())
      .filter((term: string) => term.length > 0) || [];

    // Combine AI terms with expanded terms
    const allTerms = [...expandedTerms, ...aiTerms];
    const uniqueTerms = Array.from(new Set(allTerms));
    
    return uniqueTerms.slice(0, 12); // Increased limit for better coverage
  } catch (error) {
    console.error('Error generating search terms:', error);
    // Fallback to expanded terms
    return expandSearchTerms(query);
  }
}

function scoreDocumentsByKeywords(
  query: string, 
  searchTerms: string[], 
  documents: DocumentChunk[]
): (DocumentChunk & { relevanceScore: number; matchedContent: string })[] {
  
  const scoredDocs = documents.map(doc => {
    let score = 0;
    let matchedContent = '';
    
    const queryLower = query.toLowerCase();
    const contentLower = doc.content.toLowerCase();
    const titleLower = doc.title.toLowerCase();
    const urlLower = doc.url.toLowerCase();
    
    // Keyword matching (highest priority)
    searchTerms.forEach(term => {
      const termLower = term.toLowerCase();
      
      // Check if term matches any keyword
      const keywordMatch = doc.keywords.some(keyword => 
        keyword.includes(termLower) || termLower.includes(keyword)
      );
      
      if (keywordMatch) {
        score += 20; // High score for keyword match
        
        // Find which keyword matched
        const matchedKeyword = doc.keywords.find(keyword => 
          keyword.includes(termLower) || termLower.includes(keyword)
        );
        
        if (matchedKeyword && !matchedContent) {
          matchedContent = `Keyword match: ${matchedKeyword}`;
        }
      }
      
      // Exact title match
      if (titleLower === termLower) {
        score += 25;
        matchedContent = doc.title;
      } else if (titleLower.includes(termLower)) {
        score += 15;
        if (!matchedContent) matchedContent = doc.title;
      }
      
      // URL/path matching (e.g., "node cache" in URL)
      if (urlLower.includes(termLower)) {
        score += 18;
        if (!matchedContent) matchedContent = `URL match: ${doc.url}`;
      }
      
      // Content matching
      if (contentLower.includes(termLower)) {
        score += 8;
        
        if (!matchedContent) {
          const startIndex = contentLower.indexOf(termLower);
          matchedContent = doc.content.substring(
            Math.max(0, startIndex - 50),
            Math.min(doc.content.length, startIndex + termLower.length + 50)
          );
        }
      }
    });
    
    // Exact phrase matching in title
    if (titleLower.includes(queryLower)) {
      score += 30;
      matchedContent = doc.title;
    }
    
    // Exact phrase matching in content
    if (contentLower.includes(queryLower)) {
      score += 12;
      if (!matchedContent) {
        const startIndex = contentLower.indexOf(queryLower);
        matchedContent = doc.content.substring(
          Math.max(0, startIndex - 50),
          Math.min(doc.content.length, startIndex + queryLower.length + 50)
        );
      }
    }
    
    // Use excerpt if no specific content matched
    if (!matchedContent) {
      matchedContent = doc.excerpt;
    }
    
    return {
      ...doc,
      relevanceScore: score,
      matchedContent: matchedContent.trim()
    };
  });
  
  // Filter out documents with very low scores
  return scoredDocs.filter(doc => doc.relevanceScore > 0);
}

function scoreDocumentsByAI(
  query: string, 
  searchTerms: string[], 
  documents: DocumentChunk[]
): (DocumentChunk & { relevanceScore: number; matchedContent: string })[] {
  
  const scoredDocs = documents.map(doc => {
    let score = 0;
    let matchedContent = '';
    
    const queryLower = query.toLowerCase();
    const contentLower = doc.content.toLowerCase();
    const titleLower = doc.title.toLowerCase();
    const urlLower = doc.url.toLowerCase();
    
    // AI-enhanced semantic matching
    searchTerms.forEach(term => {
      const termLower = term.toLowerCase();
      
      // Semantic similarity scoring (AI-generated terms get higher weight)
      const isAiTerm = searchTerms.length > 3 && searchTerms.indexOf(term) >= 3; // Assume AI terms come after direct terms
      
      // Title semantic matching
      if (titleLower.includes(termLower)) {
        score += isAiTerm ? 12 : 8;
        if (!matchedContent) matchedContent = doc.title;
      }
      
      // URL semantic matching
      if (urlLower.includes(termLower)) {
        score += isAiTerm ? 15 : 10;
        if (!matchedContent) matchedContent = `URL match: ${doc.url}`;
      }
      
      // Content semantic matching
      if (contentLower.includes(termLower)) {
        score += isAiTerm ? 6 : 4;
        
        if (!matchedContent) {
          const startIndex = contentLower.indexOf(termLower);
          matchedContent = doc.content.substring(
            Math.max(0, startIndex - 50),
            Math.min(doc.content.length, startIndex + termLower.length + 50)
          );
        }
      }
      
      // Keyword semantic matching
      const keywordMatch = doc.keywords.some(keyword => 
        keyword.includes(termLower) || termLower.includes(keyword)
      );
      
      if (keywordMatch) {
        score += isAiTerm ? 10 : 6;
        if (!matchedContent) {
          const matchedKeyword = doc.keywords.find(keyword => 
            keyword.includes(termLower) || termLower.includes(keyword)
          );
          matchedContent = `Semantic match: ${matchedKeyword}`;
        }
      }
    });
    
    // Exact phrase matching (higher weight for AI)
    if (titleLower.includes(queryLower)) {
      score += 20;
      matchedContent = doc.title;
    }
    
    if (contentLower.includes(queryLower)) {
      score += 10;
      if (!matchedContent) {
        const startIndex = contentLower.indexOf(queryLower);
        matchedContent = doc.content.substring(
          Math.max(0, startIndex - 50),
          Math.min(doc.content.length, startIndex + queryLower.length + 50)
        );
      }
    }
    
    // Use excerpt if no specific content matched
    if (!matchedContent) {
      matchedContent = doc.excerpt;
    }
    
    return {
      ...doc,
      relevanceScore: score,
      matchedContent: matchedContent.trim()
    };
  });
  
  return scoredDocs.filter(doc => doc.relevanceScore > 0);
}

function combineSearchResults(
  keywordResults: (DocumentChunk & { relevanceScore: number; matchedContent: string })[],
  aiResults: (DocumentChunk & { relevanceScore: number; matchedContent: string })[]
): (DocumentChunk & { relevanceScore: number; matchedContent: string })[] {
  
  const combinedMap = new Map<string, DocumentChunk & { relevanceScore: number; matchedContent: string }>();
  
  // Add keyword results with weight 0.6
  keywordResults.forEach(doc => {
    const combined = {
      ...doc,
      relevanceScore: doc.relevanceScore * 0.6,
      matchedContent: doc.matchedContent
    };
    combinedMap.set(doc.id, combined);
  });
  
  // Add AI results with weight 0.4, combining scores if document exists
  aiResults.forEach(doc => {
    const existing = combinedMap.get(doc.id);
    if (existing) {
      // Combine scores
      existing.relevanceScore += doc.relevanceScore * 0.4;
      // Prefer more specific matched content
      if (doc.matchedContent.length > existing.matchedContent.length) {
        existing.matchedContent = doc.matchedContent;
      }
    } else {
      // Add new result
      combinedMap.set(doc.id, {
        ...doc,
        relevanceScore: doc.relevanceScore * 0.4,
        matchedContent: doc.matchedContent
      });
    }
  });
  
  return Array.from(combinedMap.values());
}
