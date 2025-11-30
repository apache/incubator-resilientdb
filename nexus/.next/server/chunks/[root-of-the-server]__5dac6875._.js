module.exports = {

"[project]/.next-internal/server/app/api/graphql-tutor/analyze/route/actions.js [app-rsc] (server actions loader, ecmascript)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
}}),
"[externals]/next/dist/compiled/next-server/app-route-turbo.runtime.dev.js [external] (next/dist/compiled/next-server/app-route-turbo.runtime.dev.js, cjs)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
const mod = __turbopack_context__.x("next/dist/compiled/next-server/app-route-turbo.runtime.dev.js", () => require("next/dist/compiled/next-server/app-route-turbo.runtime.dev.js"));

module.exports = mod;
}}),
"[externals]/@opentelemetry/api [external] (@opentelemetry/api, cjs)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
const mod = __turbopack_context__.x("@opentelemetry/api", () => require("@opentelemetry/api"));

module.exports = mod;
}}),
"[externals]/next/dist/compiled/next-server/app-page-turbo.runtime.dev.js [external] (next/dist/compiled/next-server/app-page-turbo.runtime.dev.js, cjs)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
const mod = __turbopack_context__.x("next/dist/compiled/next-server/app-page-turbo.runtime.dev.js", () => require("next/dist/compiled/next-server/app-page-turbo.runtime.dev.js"));

module.exports = mod;
}}),
"[externals]/next/dist/server/app-render/work-unit-async-storage.external.js [external] (next/dist/server/app-render/work-unit-async-storage.external.js, cjs)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
const mod = __turbopack_context__.x("next/dist/server/app-render/work-unit-async-storage.external.js", () => require("next/dist/server/app-render/work-unit-async-storage.external.js"));

module.exports = mod;
}}),
"[externals]/next/dist/server/app-render/work-async-storage.external.js [external] (next/dist/server/app-render/work-async-storage.external.js, cjs)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
const mod = __turbopack_context__.x("next/dist/server/app-render/work-async-storage.external.js", () => require("next/dist/server/app-render/work-async-storage.external.js"));

module.exports = mod;
}}),
"[externals]/next/dist/server/app-render/after-task-async-storage.external.js [external] (next/dist/server/app-render/after-task-async-storage.external.js, cjs)": (function(__turbopack_context__) {

var { g: global, __dirname, m: module, e: exports } = __turbopack_context__;
{
const mod = __turbopack_context__.x("next/dist/server/app-render/after-task-async-storage.external.js", () => require("next/dist/server/app-render/after-task-async-storage.external.js"));

module.exports = mod;
}}),
"[project]/src/app/api/graphql-tutor/analyze/route.ts [app-route] (ecmascript)": ((__turbopack_context__) => {
"use strict";

var { g: global, __dirname } = __turbopack_context__;
{
__turbopack_context__.s({
    "POST": (()=>POST)
});
var __TURBOPACK__imported__module__$5b$project$5d2f$node_modules$2f$next$2f$server$2e$js__$5b$app$2d$route$5d$__$28$ecmascript$29$__ = __turbopack_context__.i("[project]/node_modules/next/server.js [app-route] (ecmascript)");
;
async function POST(req) {
    try {
        const { query } = await req.json();
        if (!query || typeof query !== "string") {
            return __TURBOPACK__imported__module__$5b$project$5d2f$node_modules$2f$next$2f$server$2e$js__$5b$app$2d$route$5d$__$28$ecmascript$29$__["NextResponse"].json({
                error: "Query is required"
            }, {
                status: 400
            });
        }
        // GraphQ-LLM backend URL (from environment or default)
        const graphqLlmUrl = process.env.GRAPHQ_LLM_API_URL || ("TURBOPACK compile-time value", "http://localhost:3001") || "http://localhost:3001";
        try {
            // Call GraphQ-LLM explanation service
            const explainResponse = await fetch(`${graphqLlmUrl}/api/explanations/explain`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    query,
                    detailed: true
                })
            });
            // Call GraphQ-LLM optimization service
            const optimizeResponse = await fetch(`${graphqLlmUrl}/api/optimizations/optimize`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    query,
                    includeSimilarQueries: true
                })
            });
            // Call GraphQ-LLM efficiency service
            const efficiencyResponse = await fetch(`${graphqLlmUrl}/api/efficiency/estimate`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    query,
                    useLiveStats: false
                })
            });
            // Parse responses
            const explanationData = explainResponse.ok ? await explainResponse.json() : null;
            const optimizationData = optimizeResponse.ok ? await optimizeResponse.json() : null;
            const efficiencyData = efficiencyResponse.ok ? await efficiencyResponse.json() : null;
            // Format explanation response for UI
            const explanation = explanationData ? {
                explanation: explanationData.explanation || "No explanation available",
                complexity: explanationData.complexity || "medium",
                recommendations: explanationData.recommendations || []
            } : {
                explanation: "Explanation service unavailable. Please ensure GraphQ-LLM backend is running.",
                complexity: "unknown",
                recommendations: []
            };
            // Format optimizations response for UI
            const optimizations = optimizationData?.suggestions ? optimizationData.suggestions.map((s)=>({
                    query: s.optimizedQuery || query,
                    explanation: s.reason || s.suggestion || "Optimization suggestion",
                    confidence: 0.8
                })) : [];
            // Format efficiency response for UI
            // Extract complexity from efficiencyData or fallback to explanationData
            const complexity = efficiencyData?.complexity || explanationData?.complexity || (efficiencyData?.queryAnalysis ? efficiencyData.queryAnalysis.fieldCount > 20 ? "high" : efficiencyData.queryAnalysis.fieldCount > 10 ? "medium" : "low" : "medium");
            const efficiency = efficiencyData ? {
                score: efficiencyData.efficiencyScore || 0,
                estimatedTime: efficiencyData.estimatedExecutionTime ? `${efficiencyData.estimatedExecutionTime}ms` : "Unknown",
                resourceUsage: efficiencyData.estimatedResourceUsage ? `CPU: ${efficiencyData.estimatedResourceUsage.cpu || "N/A"}%, Memory: ${efficiencyData.estimatedResourceUsage.memory || "N/A"}MB` : "Unknown",
                recommendations: efficiencyData.recommendations || [],
                complexity: complexity,
                similarQueries: efficiencyData.similarQueries
            } : {
                score: 0,
                estimatedTime: "Unknown",
                resourceUsage: "Unknown",
                recommendations: [],
                complexity: complexity
            };
            // Send query statistics to ResLens Middleware
            const middlewareUrl = process.env.RESLENS_MIDDLEWARE_URL || "http://localhost:3003";
            const storeUrl = `${middlewareUrl}/api/v1/queryStats/store`;
            try {
                console.log(`[Nexus] Sending query stats to middleware: ${storeUrl}`);
                console.log(`[Nexus] Efficiency complexity: ${efficiency.complexity}`);
                const middlewareResponse = await fetch(storeUrl, {
                    method: "POST",
                    headers: {
                        "Content-Type": "application/json"
                    },
                    body: JSON.stringify({
                        query,
                        efficiency: {
                            ...efficiency,
                            complexity: efficiency.complexity || "medium"
                        },
                        explanation,
                        optimizations,
                        timestamp: new Date().toISOString()
                    })
                });
                if (!middlewareResponse.ok) {
                    const errorText = await middlewareResponse.text().catch(()=>'Unknown error');
                    console.error(`[Nexus] Middleware returned ${middlewareResponse.status}: ${errorText}`);
                } else {
                    const result = await middlewareResponse.json().catch(()=>({}));
                    console.log(`[Nexus] Query stats stored successfully:`, result);
                }
            } catch (middlewareError) {
                // Log but don't fail if middleware is unavailable
                console.error("[Nexus] Failed to store query statistics in middleware:", middlewareError);
                if (middlewareError instanceof Error) {
                    console.error("[Nexus] Error details:", middlewareError.message);
                    console.error("[Nexus] Stack:", middlewareError.stack);
                }
            }
            return __TURBOPACK__imported__module__$5b$project$5d2f$node_modules$2f$next$2f$server$2e$js__$5b$app$2d$route$5d$__$28$ecmascript$29$__["NextResponse"].json({
                explanation,
                optimizations,
                efficiency
            });
        } catch (fetchError) {
            // Fallback to mock data if GraphQ-LLM backend is not available
            console.warn("GraphQ-LLM backend not available, using fallback:", fetchError);
            return __TURBOPACK__imported__module__$5b$project$5d2f$node_modules$2f$next$2f$server$2e$js__$5b$app$2d$route$5d$__$28$ecmascript$29$__["NextResponse"].json({
                explanation: {
                    explanation: `This query retrieves transaction data from ResilientDB. The query structure suggests it's fetching a single transaction by ID.`,
                    complexity: "low",
                    recommendations: [
                        "Consider adding pagination for large result sets",
                        "Use specific field selections to reduce payload size"
                    ]
                },
                optimizations: [
                    {
                        query: query,
                        explanation: "Query is already well-optimized for single transaction retrieval.",
                        confidence: 0.85
                    }
                ],
                efficiency: {
                    score: 92,
                    estimatedTime: "< 10ms",
                    resourceUsage: "Low - Single transaction lookup",
                    recommendations: [
                        "Query is efficient for single transaction retrieval"
                    ]
                }
            });
        }
    } catch (error) {
        console.error("Error analyzing query:", error);
        return __TURBOPACK__imported__module__$5b$project$5d2f$node_modules$2f$next$2f$server$2e$js__$5b$app$2d$route$5d$__$28$ecmascript$29$__["NextResponse"].json({
            error: "Failed to analyze query"
        }, {
            status: 500
        });
    }
}
}}),

};

//# sourceMappingURL=%5Broot-of-the-server%5D__5dac6875._.js.map