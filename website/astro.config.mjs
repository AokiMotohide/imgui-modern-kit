import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
	site: 'https://aokimotohide.github.io',
	base: '/imgui-modern-kit/',
	integrations: [
		starlight({
			title: 'ImKit',
			description: 'Host-owned UI modules for creative tools built with Dear ImGui.',
			defaultLocale: 'root',
			locales: {
				root: { label: '日本語', lang: 'ja' },
				en: { label: 'English', lang: 'en' },
			},
			logo: {
				src: './src/assets/imkit-mark.svg',
				alt: 'ImKit',
			},
			social: [
				{
					icon: 'github',
					label: 'GitHub',
					href: 'https://github.com/AokiMotohide/imgui-modern-kit',
				},
			],
			customCss: ['./src/styles/custom.css'],
			components: {
				Hero: './src/components/HomeHero.astro',
			},
			sidebar: [
				{
					label: 'はじめる',
					translations: { en: 'Start here' },
					items: [{ slug: 'getting-started' }],
				},
				{
					label: 'できること',
					translations: { en: 'Explore' },
					items: [
						{ slug: 'features' },
						{
							label: 'UI部品とテーマ',
							translations: { en: 'Controls and themes' },
							items: [{ slug: 'features/components' }, { slug: 'features/themes' }, { slug: 'features/design-system' }],
						},
						{
							label: 'Workflowと編集画面',
							translations: { en: 'Workflow and editors' },
							items: [{ slug: 'features/workflow' }, { slug: 'features/shell' }, { slug: 'features/node-editor' }, { slug: 'features/editor-suite' }, { slug: 'features/timeline' }],
						},
						{
							label: 'WindowFrameとIcons',
							translations: { en: 'WindowFrame and icons' },
							items: [{ slug: 'features/window-frame' }, { slug: 'features/icons' }],
						},
					],
				},
				{
					label: 'ガイド',
					translations: { en: 'Guides' },
					items: [{ label: '実例とレシピ', translations: { en: 'Examples and recipes' }, slug: 'guides/examples' }, { slug: 'guides/gallery' }, { slug: 'guide/architecture' }],
				},
				{
					label: '環境と移行',
					translations: { en: 'Platform and migration' },
					items: [{ slug: 'platform/dependencies' }, { slug: 'platform/faq' }, { slug: 'platform/troubleshooting' }, { slug: 'platform/migration' }],
				},
				{
					label: 'API',
					items: [
						{ slug: 'api' },
						{ label: 'BeginEditor', translations: { en: 'BeginEditor' }, slug: 'api/node-editor/begin-editor' },
						{ slug: 'api/node-editor' },
						{ slug: 'api/native' },
						{ label: 'Design-system API', translations: { en: 'Design-system API' }, slug: 'api/design-system' },
						{ slug: 'api/editor-suite' },
						{ slug: 'api/window-frame' },
						{ slug: 'api/widgets' },
				],
				},
			],
		}),
	],
});
