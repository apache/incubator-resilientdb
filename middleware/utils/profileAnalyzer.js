/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

const { GoogleGenerativeAI } = require("@google/generative-ai");
const { getEnv } = require("./envParser");

/**
 * Analyzes profile data using Google Gemini AI
 * @param {string} markdownContent - Profile data in markdown format
 * @returns {Promise<string>} - AI insights about the profile
 */
async function analyzeProfileContent(markdownContent) {
  try {
    const apiKey = getEnv("GOOGLE_API_KEY")
    const genAI = new GoogleGenerativeAI(apiKey);
    
    // Create a prompt for the LLM
    const prompt = `
You are an expert performance engineer. Please analyze this CPU profile data and provide insights:
1. What are the main performance bottlenecks?
2. Which functions or areas of code need optimization?
3. What specific recommendations would you make to improve performance?

Profile data:
${markdownContent}

Please provide a concise, actionable analysis based on this profile data.
`;
    
    // Get the generative model
    const model = genAI.getGenerativeModel({ model: "gemini-1.5-flash" });
    
    // Make the API call
    const result = await model.generateContent({
      contents: [
        {
          role: "user",
          parts: [{ text: prompt }]
        }
      ],
      generationConfig: {
        temperature: 0.2,
        maxOutputTokens: 2000,
      }
    });

    const response = result.response;
    return response.text();
  } catch (error) {
    console.error("Error analyzing profile:", error);
    throw new Error(`Failed to analyze profile: ${error.message}`);
  }
}

module.exports = {
  analyzeProfileContent
};
