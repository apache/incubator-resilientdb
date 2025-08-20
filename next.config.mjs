import createMDX from "@next/mdx";
import withLlamaIndex from "llamaindex/next";
import rehypeHighlight from "rehype-highlight";
import remarkGfm from "remark-gfm";

/** @type {import('next').NextConfig} */
const nextConfig = {
  // Allow .mdx extensions for files
  pageExtensions: ["js", "jsx", "md", "mdx", "ts", "tsx"],
  // Exclude archive and experimental files from type checking
  typescript: {
    ignoreBuildErrors: false,
  },
  eslint: {
    ignoreDuringBuilds: false,
  },
  // Your Next.js config options go here
};

const withMDX = createMDX({
  // Add markdown plugins here
  options: {
    remarkPlugins: [remarkGfm],
    rehypePlugins: [rehypeHighlight],
  },
});

// Combine MDX and LlamaIndex configs
export default withLlamaIndex(withMDX(nextConfig));
