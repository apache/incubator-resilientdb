require("dotenv").config();
const { Resend } = require("resend");

const resend = new Resend(process.env.RESEND_API_KEY);

async function testEmail() {
  console.log("Testing Resend email functionality...");
  console.log("API Key present:", !!process.env.RESEND_API_KEY);
  console.log("API Key starts with:", process.env.RESEND_API_KEY?.substring(0, 10) + "...");

  try {
    const result = await resend.emails.send({
      from: "Acme <onboarding@resend.dev>",
      to: ["developer@resilientdb.com"], 
      subject: "🧪 Test Email from ResilientDB Monitor",
      html: `
        <div style="font-family: monospace; padding: 20px;">
          <h1>Test Email</h1>
          <p>This is a test email from your ResilientDB monitoring system.</p>
          <p>If you receive this, the email configuration is working correctly!</p>
        </div>
      `,
      text: "This is a test email from your ResilientDB monitoring system. If you receive this, the email configuration is working correctly!"
    });

    console.log("✅ Email sent successfully!");
    console.log("Result:", result);
  } catch (error) {
    console.error("❌ Email sending failed:");
    console.error(error);
  }
}

testEmail();