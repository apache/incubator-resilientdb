# Mantine Next.js + Nextra template


<img width="1536" alt="image" src="https://github.com/user-attachments/assets/eac2e76d-0c63-4429-bb93-b75476e55216" />



This is a template for [Next.js](https://nextjs.org/) app router + [Mantine](https://mantine.dev/) + [Nextra](https://nextra.site/) documentation site.

## Features

This template comes with the following features:

- [PostCSS](https://postcss.org/) with [mantine-postcss-preset](https://mantine.dev/styles/postcss-preset)
- [TypeScript](https://www.typescriptlang.org/)
- [Storybook](https://storybook.js.org/)
- [Jest](https://jestjs.io/) setup with [React Testing Library](https://testing-library.com/docs/react-testing-library/intro)
- ESLint setup with [eslint-config-mantine](https://github.com/mantinedev/eslint-config-mantine)
- Provides API example in `/api/version`

## Nextra Features

- [Nextra](https://nextra.site/) documentation site with Mantine theme
- Sync Dark mode between documentation and application (Mantine/Nextra)
- Customizable components in `components` folder
- Provides new Navigation and Footer components for Nextra documentation site

## Folder structure

- `components` – shared components 
    - you can use them in both documentation and application
    - you may customize them to fit your needs
- `content` – Nextra documentation site (.mdx and _meta.json files)


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
