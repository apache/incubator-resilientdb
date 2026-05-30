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

require("dotenv").config();
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

async function testEmail() {
  console.log("Testing Nodemailer email functionality...");
  console.log("Email user configured:", !!process.env.EMAIL_USER);
  console.log("Email provider:", process.env.EMAIL_PROVIDER || "gmail");

  try {
    const transporter = createTransporter();

    // Verify transporter configuration
    await transporter.verify();
    console.log("✅ SMTP connection verified successfully");

    const fromEmail = process.env.EMAIL_FROM || process.env.EMAIL_USER;
    const testEmail = process.env.TEST_EMAIL || "developer@resilientdb.com";

    console.log("Sending from:", fromEmail);
    console.log("Sending to:", testEmail);

    const mailOptions = {
      from: fromEmail,
      to: testEmail,
      subject: "🧪 Test Email from ResilientDB Monitor",
      html: `
        <div style="font-family: monospace; padding: 20px;">
          <h1>Test Email</h1>
          <p>This is a test email from your ResilientDB monitoring system.</p>
          <p>If you receive this, the email configuration is working correctly!</p>
          <p><strong>Provider:</strong> ${process.env.EMAIL_PROVIDER || "gmail"}</p>
        </div>
      `,
      text: "This is a test email from your ResilientDB monitoring system. If you receive this, the email configuration is working correctly!"
    };

    const result = await transporter.sendMail(mailOptions);

    console.log("✅ Email sent successfully!");
    console.log("Message ID:", result.messageId);
    console.log("Result:", result);
  } catch (error) {
    console.error("❌ Email sending failed:");
    console.error(error);
  }
}

testEmail();