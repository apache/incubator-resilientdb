# Beacon – Next‑Gen Docs for the ResilientDB Ecosystem

Beacon is a modern documentation and application shell built with the Next.js App Router, Mantine UI, and the Nextra Docs theme. It powers the ResilientDB docs experience with a custom landing page, synchronized dark/light theming between Mantine and Nextra, and a growing set of interactive examples (TypeScript and Python playgrounds, IDE embeds, etc.).

<img width="1536" alt="image" src="https://github.com/user-attachments/assets/eac2e76d-0c63-4429-bb93-b75476e55216" />

Key goals:
- Provide a fast, consistent docs experience for the ResilientDB ecosystem
- Keep the UI simple and readable while supporting rich, interactive content
- Ensure theming is consistent across the landing page and Nextra-powered docs

## Features

This project includes the following:

- [PostCSS](https://postcss.org/) with [mantine-postcss-preset](https://mantine.dev/styles/postcss-preset)
- [TypeScript](https://www.typescriptlang.org/)
- [Storybook](https://storybook.js.org/)
- [Jest](https://jestjs.io/) setup with [React Testing Library](https://testing-library.com/docs/react-testing-library/intro)
- ESLint setup with [eslint-config-mantine](https://github.com/mantinedev/eslint-config-mantine)
- Provides API example in `/api/version`

## Nextra Features

- [Nextra](https://nextra.site/) documentation site using the Docs theme
- Theme sync between Mantine and Nextra (dark mode by default)
- Customizable UI in `components/` (Navbar, Footer, ColorScheme controls)
- Custom landing page in `app/page.tsx` with Mantine components
- Interactive examples (TypeScript/Python playgrounds) and MDX-driven content

## Folder structure

- `components` – shared components 
    - you can use them in both documentation and application
    - you may customize them to fit your needs
- `content` – Nextra documentation site (.mdx and _meta files)


## Getting started

Install dependencies and run the dev server:

```bash
npm i
npm run dev
```

Build for production:

```bash
npm run build
```

## npm scripts

### Build and dev scripts

- `dev` – start dev server
- `build` – bundle application for production
- `analyze` – analyzes application bundle with [@next/bundle-analyzer](https://www.npmjs.com/package/@next/bundle-analyzer)

### Testing scripts

- `typecheck` – checks TypeScript types
- `lint` – runs ESLint
- `prettier:check` – checks files with Prettier
- `jest` – runs jest tests
- `jest:watch` – starts jest watch
- `test` – runs `jest`, `prettier:check`, `lint` and `typecheck` scripts

### Other scripts

- `storybook` – starts storybook dev server
- `storybook:build` – build production storybook bundle to `storybook-static`
- `prettier:write` – formats all files with Prettier

## Contributing

Issues and PRs are welcome. Keep edits focused and match the existing code style. For UI changes, prefer small, composable CSS updates (ideally inside `app/global.css` under `@layer nextra`) that play nicely with Nextra’s cascade.

## TODO

- Fix light mode (align backgrounds and content in Nextra with landing page)
