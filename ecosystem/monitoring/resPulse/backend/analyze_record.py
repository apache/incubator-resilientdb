#!/usr/bin/env python3
# analyze_record.py
# On-demand analysis of an existing performance record against a given baseline.
# Input:  JSON on stdin — { record: {...}, baseline: {...} | null, period_label: "..." }
# Output: Analysis JSON to stdout

import json
import sys
import os
from openai import OpenAI

# Load environment variables from .env file
def load_env_file():
    env_path = os.path.join(os.path.dirname(__file__), '.env')
    if os.path.exists(env_path):
        with open(env_path, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#') and '=' in line:
                    key, value = line.split('=', 1)
                    os.environ[key] = value

# Load .env file at startup
load_env_file()


def pct_change(current, baseline):
    if not baseline:
        return None
    return round(((current - baseline) / baseline) * 100, 1)


def get_ai_analysis(record, baseline, period_label=""):
    """Get AI-powered analysis from Deepseek API"""
    api_key = os.environ.get("DEEPSEEK_API_KEY")
    if not api_key:
        return {
            "diagnosis": ["AI analysis unavailable: DEEPSEEK_API_KEY not configured"],
            "recommendations": ["Please set DEEPSEEK_API_KEY environment variable to enable AI-powered analysis"],
            "overall_status": "configuration_error"
        }

    try:
        client = OpenAI(
            api_key=api_key,
            base_url="https://api.deepseek.com",
        )

        # Prepare data for AI analysis
        analysis_data = {
            "current_metrics": record,
            "baseline_metrics": baseline,
            "period_label": period_label
        }

        # Create prompt for AI analysis
        prompt = f"""
You are a ResilientDB performance expert analyzing test results. Write a clear, cohesive analysis report.

Current Performance Metrics:
- Average Latency: {record.get('avg_latency_ms', 0)}ms
- Throughput: {record.get('throughput_rps', 0)} req/s
- Success Rate: {record.get('success_rate', 0)}%
- P50 Latency: {record.get('total_latency', {}).get('p50', 0)}ms
- P99 Latency: {record.get('total_latency', {}).get('p99', 0)}ms
- Server Wait Time: {record.get('consensus_time_ms', {}).get('mean', 0)}ms
- TCP Connect Time: {record.get('tcp_connect_ms', {}).get('mean', 0)}ms

{f"Baseline Comparison (vs {period_label}):" if baseline else "No historical baseline available."}
{f"Historical Average Latency: {(baseline.get('avg_latency_ms') or {}).get('mean', 0)}ms" if baseline else ""}
{f"Historical Average Throughput: {(baseline.get('throughput_rps') or {}).get('mean', 0)} req/s" if baseline else ""}

Write a professional performance analysis report with these sections:

PERFORMANCE SUMMARY:
Write 2-3 sentences summarizing the overall system performance and key findings.

KEY FINDINGS:
List 3-4 main observations about bottlenecks, performance patterns, or issues.

RECOMMENDATIONS:
Provide 3-4 specific, actionable recommendations for improvement.

STATUS:
Choose one status: "stable", "minor_warning", "needs_attention", "possible_regression", or "configuration_error"

Write in clear, professional language. Avoid bullet points and fragmented text. Make it flow naturally as a cohesive report.
"""

        response = client.chat.completions.create(
            model="deepseek-v4-flash",
            messages=[
                {"role": "system", "content": "You are an expert ResilientDB performance analyst. Provide technical, actionable insights about PBFT consensus and database performance."},
                {"role": "user", "content": prompt}
            ],
            temperature=0.1,
            max_tokens=1000,
            timeout=20.0
        )

        ai_response = response.choices[0].message.content

        # Parse AI response into structured format - use the full response as diagnosis for better readability
        diagnosis = [ai_response]
        recommendations = []
        overall_status = "stable"

        # Extract status from the response
        for status in ["possible_regression", "needs_attention", "minor_warning", "stable", "configuration_error"]:
            if status in ai_response.lower():
                overall_status = status
                break

        # Extract recommendations section for separate display if needed
        if "RECOMMENDATIONS" in ai_response.upper():
            parts = ai_response.upper().split("RECOMMENDATIONS")
            if len(parts) > 1:
                # Find the actual recommendations text
                rec_start = ai_response.upper().find("RECOMMENDATIONS")
                rec_text = ai_response[rec_start:]
                # Look for the end of recommendations (STATUS section)
                status_start = rec_text.upper().find("STATUS")
                if status_start > 0:
                    rec_text = rec_text[:status_start]

                # Clean up and extract just the recommendations content
                rec_text = rec_text.replace("RECOMMENDATIONS", "").replace("**RECOMMENDATIONS**", "").strip()
                if rec_text.startswith(":"):
                    rec_text = rec_text[1:].strip()

                if rec_text:
                    recommendations = [rec_text]

        return {
            "diagnosis": diagnosis,
            "recommendations": recommendations,
            "overall_status": overall_status
        }

    except Exception as e:
        error_msg = str(e)
        if "timeout" in error_msg.lower() or "timed out" in error_msg.lower():
            return {
                "diagnosis": [f"AI analysis timed out: {error_msg}"],
                "recommendations": ["AI service temporarily unavailable. Analysis will retry automatically on next request."],
                "overall_status": "timeout_error"
            }
        else:
            return {
                "diagnosis": [f"AI analysis failed: {error_msg}"],
                "recommendations": ["Check DEEPSEEK_API_KEY configuration and network connectivity"],
                "overall_status": "configuration_error"
            }


def analyze(record, baseline, period_label=""):
    avg_latency       = record.get("avg_latency_ms", 0) or 0
    throughput_val    = record.get("throughput_rps", 0) or 0
    success_rate      = record.get("success_rate", 0) or 0
    total_stats       = record.get("total_latency") or {}
    server_wait_stats = record.get("consensus_time_ms") or {}
    tcp_stats         = record.get("tcp_connect_ms") or {}
    transfer_stats    = record.get("transfer_time_ms") or {}

    p50              = total_stats.get("p50", 0) or 0
    p99              = total_stats.get("p99", 0) or 0
    stddev           = total_stats.get("stddev", 0) or 0
    server_wait_mean = server_wait_stats.get("mean", 0) or 0
    tcp_mean         = tcp_stats.get("mean", 0) or 0
    transfer_mean    = transfer_stats.get("mean", 0) or 0

    warnings         = []
    baseline_summary = {}

    if baseline:
        hist_avg_latency     = (baseline.get("avg_latency_ms") or {}).get("mean", 0) or 0
        hist_avg_throughput  = (baseline.get("throughput_rps") or {}).get("mean", 0) or 0
        hist_avg_server_wait = (baseline.get("consensus_time_ms") or {}).get("mean", 0) or 0
        record_count         = baseline.get("count", 0)
        period_start         = baseline.get("period_start")

        baseline_summary = {
            "source":                        "mongodb",
            "record_count":                  record_count,
            "period_start":                  period_start,
            "historical_avg_latency_ms":     round(hist_avg_latency, 2),
            "historical_avg_throughput_rps": round(hist_avg_throughput, 2),
            "historical_avg_server_wait_ms": round(hist_avg_server_wait, 2),
        }

        latency_change     = pct_change(avg_latency,     hist_avg_latency)
        throughput_change  = pct_change(throughput_val,  hist_avg_throughput)
        server_wait_change = pct_change(server_wait_mean, hist_avg_server_wait)
    else:
        warnings.append(f"No baseline data available for the selected period.")
        latency_change = throughput_change = server_wait_change = None
        hist_avg_latency = hist_avg_throughput = 0

    # Get AI-powered analysis
    ai_analysis = get_ai_analysis(record, baseline, period_label)
    diagnosis = ai_analysis["diagnosis"]
    recommendations = ai_analysis["recommendations"]
    overall_status = ai_analysis["overall_status"]

    # Add traditional baseline comparison data for context
    if baseline:
        period_str = f"the {period_label} " if period_label else "the historical "

        if latency_change is not None:
            if latency_change > 25:
                diagnosis.append(f"Baseline comparison: Average latency is {latency_change}% higher than {period_str}baseline.")
            elif latency_change < -25:
                diagnosis.append(f"Baseline comparison: Average latency is {abs(latency_change)}% lower than {period_str}baseline.")

        if throughput_change is not None:
            if throughput_change < -25:
                diagnosis.append(f"Baseline comparison: Throughput is {abs(throughput_change)}% below {period_str}baseline.")
            elif throughput_change > 25:
                diagnosis.append(f"Baseline comparison: Throughput is {throughput_change}% above {period_str}baseline.")

        if server_wait_change is not None and server_wait_change > 25:
            diagnosis.append(f"Baseline comparison: Server-side wait time is {server_wait_change}% higher than {period_str}baseline.")

    return {
        "overall_status":              overall_status,
        "warnings":                    warnings,
        "diagnosis":                   diagnosis,
        "recommendations":             recommendations,
        "historical_baseline_summary": baseline_summary,
        "historical_percent_changes":  {
            "avg_latency_change_pct":  latency_change,
            "throughput_change_pct":   throughput_change,
            "server_wait_change_pct":  server_wait_change,
        },
    }


if __name__ == "__main__":
    inp          = json.loads(sys.stdin.read())
    record       = inp["record"]
    baseline     = inp.get("baseline")
    period_label = inp.get("period_label", "")
    print(json.dumps(analyze(record, baseline, period_label)))
