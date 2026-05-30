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

const nodemailer = require("nodemailer");

// Create transporter based on provider
function createTransporter() {
  const provider = process.env.EMAIL_PROVIDER || "gmail";

  switch (provider.toLowerCase()) {
    case "gmail":
      return nodemailer.createTransport({
        service: "gmail",
        auth: {
          user: process.env.EMAIL_USER,
          pass: process.env.EMAIL_PASSWORD, // Use app password for Gmail
        },
      });

    case "outlook":
    case "hotmail":
      return nodemailer.createTransport({
        service: "hotmail",
        auth: {
          user: process.env.EMAIL_USER,
          pass: process.env.EMAIL_PASSWORD,
        },
      });

    case "smtp":
    default:
      return nodemailer.createTransport({
        host: process.env.SMTP_HOST,
        port: process.env.SMTP_PORT || 587,
        secure: process.env.SMTP_SECURE === "true", // true for 465, false for other ports
        auth: {
          user: process.env.EMAIL_USER,
          pass: process.env.EMAIL_PASSWORD,
        },
      });
  }
}

async function sendRegressionAlert(config, regressions, testData) {
  console.log("[alerting] sendRegressionAlert called with:", {
    hasConfig: !!config,
    email: config?.email,
    regressionsCount: regressions?.length,
    hasTestData: !!testData
  });

  if (!config || !config.email) {
    console.log("[alerting] No email configuration found, skipping alert");
    return;
  }

  if (!testData) {
    console.log("[alerting] No test data provided, skipping alert");
    return;
  }

  if (!regressions || regressions.length === 0) {
    console.log("[alerting] No regressions provided, skipping alert");
    return;
  }

  console.log("[alerting] Proceeding to send email alert...");

  try {
    const subject = "🚨 Performance Regression Detected - ResilientDB";

    // Build regression summary
    const regressionSummary = regressions.map(r =>
      `• ${r.metric}: ${r.change > 0 ? '+' : ''}${r.change.toFixed(1)}% (threshold: ${r.threshold}%)`
    ).join('\n');

    const htmlContent = `
      <div style="font-family: 'JetBrains Mono', monospace; background: #0a0c0f; color: #e2e8f0; padding: 24px;">
        <div style="max-width: 600px; margin: 0 auto;">
          <h1 style="color: #00d4ff; margin-bottom: 24px;">🚨 Performance Regression Alert</h1>

          <div style="background: #111318; border: 1px solid #1e2430; border-radius: 8px; padding: 20px; margin-bottom: 20px;">
            <h2 style="color: #ff4757; margin-top: 0;">Detected Regressions:</h2>
            <div style="background: #181c23; padding: 12px; border-radius: 4px; font-size: 14px; white-space: pre-line;">
${regressionSummary}
            </div>
          </div>

          <div style="background: #111318; border: 1px solid #1e2430; border-radius: 8px; padding: 20px; margin-bottom: 20px;">
            <h3 style="color: #00d4ff; margin-top: 0;">Test Details:</h3>
            <p><strong>Timestamp:</strong> ${new Date(testData.timestamp).toLocaleString()}</p>
            <p><strong>Version:</strong> ${testData.version || 'N/A'}</p>
            <p><strong>Duration:</strong> ${testData.duration_ms || 'N/A'}ms</p>
            <p><strong>Success Rate:</strong> ${testData.success_rate ? (testData.success_rate * 100).toFixed(1) : 'N/A'}%</p>
          </div>

          <div style="background: #111318; border: 1px solid #1e2430; border-radius: 8px; padding: 20px;">
            <h3 style="color: #00d4ff; margin-top: 0;">Current Metrics:</h3>
            <table style="width: 100%; border-collapse: collapse;">
              <tr style="border-bottom: 1px solid #252d3a;">
                <td style="padding: 8px 0; color: #7a8aa0;">Latency:</td>
                <td style="padding: 8px 0;">${testData.avg_latency_ms ? testData.avg_latency_ms.toFixed(2) : 'N/A'}ms</td>
              </tr>
              <tr style="border-bottom: 1px solid #252d3a;">
                <td style="padding: 8px 0; color: #7a8aa0;">Consensus:</td>
                <td style="padding: 8px 0;">${testData.consensus_time_ms?.mean ? testData.consensus_time_ms.mean.toFixed(2) : 'N/A'}ms</td>
              </tr>
              <tr style="border-bottom: 1px solid #252d3a;">
                <td style="padding: 8px 0; color: #7a8aa0;">Throughput:</td>
                <td style="padding: 8px 0;">${testData.throughput_rps ? testData.throughput_rps.toFixed(2) : 'N/A'} r/s</td>
              </tr>
              <tr>
                <td style="padding: 8px 0; color: #7a8aa0;">Success Rate:</td>
                <td style="padding: 8px 0;">${testData.success_rate ? (testData.success_rate * 100).toFixed(1) : 'N/A'}%</td>
              </tr>
            </table>
          </div>

          <p style="margin-top: 24px; font-size: 12px; color: #4a5568;">
            This alert was triggered by your ResilientDB performance monitoring system.
            <br>View the dashboard for more details and historical trends.
          </p>
        </div>
      </div>
    `;

    const textContent = `
🚨 Performance Regression Detected - ResilientDB

Detected Regressions:
${regressionSummary}

Test Details:
• Timestamp: ${new Date(testData.timestamp).toLocaleString()}
• Version: ${testData.version || 'N/A'}
• Duration: ${testData.duration_ms || 'N/A'}ms
• Success Rate: ${testData.success_rate ? (testData.success_rate * 100).toFixed(1) : 'N/A'}%

Current Metrics:
• Latency: ${testData.avg_latency_ms ? testData.avg_latency_ms.toFixed(2) : 'N/A'}ms
• Consensus: ${testData.consensus_time_ms?.mean ? testData.consensus_time_ms.mean.toFixed(2) : 'N/A'}ms
• Throughput: ${testData.throughput_rps ? testData.throughput_rps.toFixed(2) : 'N/A'} r/s
• Success Rate: ${testData.success_rate ? (testData.success_rate * 100).toFixed(1) : 'N/A'}%

This alert was triggered by your ResilientDB performance monitoring system.
    `;

    console.log("[alerting] Attempting to send email to:", config.email);
    console.log("[alerting] Email user configured:", !!process.env.EMAIL_USER);

    const transporter = createTransporter();

    // Verify transporter configuration
    await transporter.verify();
    console.log("[alerting] SMTP connection verified successfully");

    const fromEmail = process.env.EMAIL_FROM || process.env.EMAIL_USER;

    const mailOptions = {
      from: fromEmail,
      to: config.email,
      subject,
      html: htmlContent,
      text: textContent,
    };

    const result = await transporter.sendMail(mailOptions);

    console.log("[alerting] Regression alert sent successfully:", result);
    return result;

  } catch (error) {
    console.error("[alerting] Failed to send regression alert:", error);
    throw error;
  }
}

module.exports = { sendRegressionAlert };